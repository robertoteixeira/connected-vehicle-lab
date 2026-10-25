//
//  BluetoothManager.swift
//  ConnectedVehicle
//
//  Created by Roberto Teixeira on 18/10/2026.
//

import Combine
import CoreBluetooth
import Foundation

@MainActor
final class BluetoothManager: NSObject,
                              ObservableObject,
                              CBCentralManagerDelegate,
                              CBPeripheralDelegate {
    private var centralManager: CBCentralManager!
    private var vehiclePeripheral: CBPeripheral?
    private var vehicleStateCharacteristic: CBCharacteristic?
    private var wifiConfigCharacteristic: CBCharacteristic?
    
    private let vehicleServiceUUID = CBUUID(
        string: "7B9F0001-6C5A-4E8F-9D3A-2F1B7C8E0001"
    )
    private let vehicleStateCharacteristicUUID = CBUUID(
        string: "7B9F0002-6C5A-4E8F-9D3A-2F1B7C8E0001"
    )
    
    private let provisioningServiceUUID = CBUUID(
        string: "7B9F0010-6C5A-4E8F-9D3A-2F1B7C8E0001"
    )
    
    private let deviceIdentityCharacteristicUUID = CBUUID(
        string: "7B9F0011-6C5A-4E8F-9D3A-2F1B7C8E0001"
    )
    
    private let wifiConfigCharacteristicUUID = CBUUID(
        string: "7B9F0012-6C5A-4E8F-9D3A-2F1B7C8E0001"
    )
    
    @Published private(set) var vehicleState = VehicleState()
    @Published private(set) var isConnected = false
    @Published private(set) var deviceIdentity: DeviceIdentity?
    @Published private(set) var hasReceivedTelemetry = false
    
    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }
    
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        switch central.state {
        case .poweredOn:
            print("BLE powered on")
            
            central.scanForPeripherals(
                withServices: [vehicleServiceUUID],
                options: nil
            )
        case .poweredOff:
            print("BLE powered off")
        case .unauthorized:
            print("BLE unauthorized")
        case .unsupported:
            print("BLE unsupported")
        case .resetting:
            print("BLE resetting")
        case .unknown:
            print("BLE state unknown")
        @unknown default:
            print("BLE unknown future state")
        }
    }
    
    func centralManager(
        _ central: CBCentralManager,
        didDiscover peripheral: CBPeripheral,
        advertisementData: [String: Any],
        rssi RSSI: NSNumber
    ) {
        print("Discovered: \(peripheral.name ?? "Unknown") RSSI: \(RSSI)")
        
        vehiclePeripheral = peripheral
        central.stopScan()
        central.connect(peripheral, options: nil)
    }
    
    func centralManager(
        _ central: CBCentralManager,
        didConnect peripheral: CBPeripheral
    ) {
        hasReceivedTelemetry = false
        isConnected = true
        
        print("Connected: \(peripheral.name ?? "Unknown")")
        
        peripheral.delegate = self
        peripheral.discoverServices([vehicleServiceUUID, provisioningServiceUUID])
    }
    
    func centralManager(
        _ central: CBCentralManager,
        didFailToConnect peripheral: CBPeripheral,
        error: Error?
    ) {
        isConnected = false
        hasReceivedTelemetry = false
        
        if let error {
            print("Connection failed: \(error)")
        } else {
            print("Connection failed: \(peripheral.name ?? "Unknown")")
        }
        
        vehiclePeripheral = nil
        vehicleStateCharacteristic = nil
        wifiConfigCharacteristic = nil
        deviceIdentity = nil
        
        scheduleScanRetry()
    }
    
    func centralManager(
        _ central: CBCentralManager,
        didDisconnectPeripheral peripheral: CBPeripheral,
        error: (any Error)?
    ) {
        isConnected = false
        hasReceivedTelemetry = false
        
        if let error {
            print("Disconnected with error: \(error)")
        } else {
            print("Disconnected: \(peripheral.name ?? "Unknown")")
        }
        
        vehiclePeripheral = nil
        vehicleStateCharacteristic = nil
        wifiConfigCharacteristic = nil
        deviceIdentity = nil
        
        scheduleScanRetry()
    }
    
    func peripheral(
        _ peripheral: CBPeripheral,
        didDiscoverServices error: Error?
    ) {
        if let error {
            print("Service discovery failed: \(error)")
            return
        }

        guard let services = peripheral.services else {
            return
        }

        for service in services {
            print("Discovered service: \(service.uuid)")
            if service.uuid == vehicleServiceUUID {
                peripheral.discoverCharacteristics(
                    [vehicleStateCharacteristicUUID],
                    for: service
                )
            } else if service.uuid == provisioningServiceUUID {
                peripheral.discoverCharacteristics(
                    [
                        deviceIdentityCharacteristicUUID,
                        wifiConfigCharacteristicUUID
                    ],
                    for: service
                )
            }
        }
    }
    
    func peripheral(
        _ peripheral: CBPeripheral,
        didDiscoverCharacteristicsFor service: CBService,
        error: Error?
    ) {
        if let error {
            print("Characteristic discovery failed: \(error)")
            return
        }

        guard let characteristics = service.characteristics else {
            return
        }

        for characteristic in characteristics {
            print("Discovered characteristic: \(characteristic.uuid)")
            
            if characteristic.uuid == vehicleStateCharacteristicUUID {
                vehicleStateCharacteristic = characteristic
                peripheral.readValue(for: characteristic)
                peripheral.setNotifyValue(true, for: characteristic)
            } else if characteristic.uuid == deviceIdentityCharacteristicUUID {
                peripheral.readValue(for: characteristic)
            } else if characteristic.uuid == wifiConfigCharacteristicUUID {
                wifiConfigCharacteristic = characteristic
                print("Wi-Fi configuration characteristic ready")
                
                sendWifiConfiguration(
                    ssid: "Dpto 1B",
                    password: "tigoapsa1b"
                )
            }
        }
    }
    
    func peripheral(
        _ peripheral: CBPeripheral,
        didUpdateValueFor characteristic: CBCharacteristic,
        error: Error?
    ) {
        if let error {
            print("Value update failed: \(error)")
            return
        }

        if characteristic.uuid == deviceIdentityCharacteristicUUID {
            guard let data = characteristic.value,
                  let payload = String(data: data, encoding: .utf8),
                  let identity = DeviceIdentity(blePayload: payload) else {
                return
            }
            
            deviceIdentity = identity
            print("Device identity: \(identity)")
            return
        }
        
        guard characteristic.uuid == vehicleStateCharacteristicUUID,
              let data = characteristic.value,
              let payload = String(data: data, encoding: .utf8) else {
            return
        }
        
        guard let state = VehicleState(blePayload: payload) else {
            print("Failed to parse vehicle state: \(payload)")
            return
        }
        
        vehicleState = state
        hasReceivedTelemetry = true

        print("Vehicle state: \(state)")
    }
    
    func peripheral(
        _ peripheral: CBPeripheral,
        didUpdateNotificationStateFor characteristic: CBCharacteristic,
        error: Error?
    ) {
        if let error {
            print("Notification subscription failed: \(error)")
            return
        }

        print(
            "Notifications \(characteristic.isNotifying ? "enabled" : "disabled") " +
            "for \(characteristic.uuid)"
        )
    }
    
    func sendWifiConfiguration(ssid: String, password: String) {
        guard let peripheral = vehiclePeripheral,
              let characteristic = wifiConfigCharacteristic else {
            print("Wi-Fi configuration characteristic unavailable")
            return
        }

        let payload = "ssid=\(ssid),password=\(password)"

        guard let data = payload.data(using: .utf8) else {
            return
        }

        peripheral.writeValue(
            data,
            for: characteristic,
            type: .withResponse
        )
    }
    
    private func scheduleScanRetry() {
        Task { [weak self] in
            try? await Task.sleep(for: .seconds(2))
            
            guard let self,
                  self.centralManager.state == .poweredOn,
                  self.vehiclePeripheral == nil else {
                return
            }
            
            print("Retrying BLE scan")
            
            self.centralManager.scanForPeripherals(withServices: [self.vehicleServiceUUID], options: nil)
        }
    }
}
