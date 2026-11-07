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
    private var wifiResultDeadlineTask: Task<Void, Never>?
    private var connectionDeadlineTask: Task<Void, Never>?
    private var scanRetryTask: Task<Void, Never>?
    
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
    
    private let wifiStatusCharacteristicUUID = CBUUID(
        string: "7B9F0013-6C5A-4E8F-9D3A-2F1B7C8E0001"
    )
    
    @Published private(set) var vehicleState = VehicleState()
    @Published private(set) var deviceIdentity: DeviceIdentity?
    @Published private(set) var hasReceivedTelemetry = false
    @Published private(set) var canConfigureWifi = false
    @Published private(set) var isSendingWifiConfiguration = false
    @Published private(set) var wifiWriteMessage: String?
    @Published private(set) var wifiProvisioningState: WifiProvisioningState?
    @Published private(set) var isWifiProvisioningPending = false
    @Published private(set) var bluetoothState: CBManagerState = .unknown
    @Published private(set) var connectionPhase: BLEConnectionPhase = .idle

    var isConnected: Bool {
        connectionPhase == .connected
    }
    
    var canStartWifiConfiguration: Bool {
        isConnected &&
        canConfigureWifi &&
        !isSendingWifiConfiguration &&
        !isWifiProvisioningPending &&
        wifiProvisioningState != nil &&
        wifiProvisioningState != .connecting
    }
    
    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }
    
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        bluetoothState = central.state
        
        if central.state != .poweredOn {
            clearConnectionState()
        }
        
        switch central.state {
        case .poweredOn:
            print("BLE powered on")
            
            cancelScanRetry()
            connectionPhase = .scanning
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
        guard vehiclePeripheral == nil else {
            return
        }
        
        cancelScanRetry()

        print("Discovered: \(peripheral.name ?? "Unknown") RSSI: \(RSSI)")
        
        vehiclePeripheral = peripheral
        central.stopScan()
        connectionPhase = .connecting
        central.connect(peripheral, options: nil)
        startConnectionDeadline(for: peripheral)
    }
    
    func centralManager(
        _ central: CBCentralManager,
        didConnect peripheral: CBPeripheral
    ) {
        cancelScanRetry()
        cancelConnectionDeadline()
        hasReceivedTelemetry = false
        connectionPhase = .connected
        
        print("Connected: \(peripheral.name ?? "Unknown")")
        
        peripheral.delegate = self
        peripheral.discoverServices([vehicleServiceUUID, provisioningServiceUUID])
    }
    
    func centralManager(
        _ central: CBCentralManager,
        didFailToConnect peripheral: CBPeripheral,
        error: Error?
    ) {
        clearConnectionState()
        
        if let error {
            print("Connection failed: \(error)")
        } else {
            print("Connection failed: \(peripheral.name ?? "Unknown")")
        }
                
        scheduleScanRetry()
    }
    
    func centralManager(
        _ central: CBCentralManager,
        didDisconnectPeripheral peripheral: CBPeripheral,
        error: (any Error)?
    ) {
        clearConnectionState()
        
        if let error {
            print("Disconnected with error: \(error)")
        } else {
            print("Disconnected: \(peripheral.name ?? "Unknown")")
        }
        
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
                        wifiConfigCharacteristicUUID,
                        wifiStatusCharacteristicUUID
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
                canConfigureWifi = true
                
                print("Wi-Fi configuration characteristic ready")
            } else if characteristic.uuid == wifiStatusCharacteristicUUID {
                peripheral.setNotifyValue(true, for: characteristic)
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

        if characteristic.uuid == wifiStatusCharacteristicUUID {
            guard peripheral === vehiclePeripheral,
                  let data = characteristic.value,
                  let payload = String(data: data, encoding: .utf8),
                  let state = WifiProvisioningState(rawValue: payload) else {
                print("Invalid Wi-Fi provisioning status")
                return
            }
            
            wifiProvisioningState = state
            
            if state == .connected || state == .timedOut {
                cancelWifiResultDeadline()
                isWifiProvisioningPending = false
                wifiWriteMessage = nil
            }
            
            print("Wi-Fi provisioning status: \(state.rawValue)")
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
        
        if characteristic.uuid == wifiStatusCharacteristicUUID,
           characteristic.isNotifying
        {
            peripheral.readValue(for: characteristic)
        }

        print(
            "Notifications \(characteristic.isNotifying ? "enabled" : "disabled") " +
            "for \(characteristic.uuid)"
        )
    }
    
    func peripheral(
        _ peripheral: CBPeripheral,
        didWriteValueFor characteristic: CBCharacteristic,
        error: Error?
    ) {
        guard peripheral === vehiclePeripheral,
              characteristic.uuid == wifiConfigCharacteristicUUID,
              isSendingWifiConfiguration else {
            return
        }

        isSendingWifiConfiguration = false

        if let error {
            cancelWifiResultDeadline()
            isWifiProvisioningPending = false
            
            wifiWriteMessage = "Could not send credentials. Please try again."
            print("Wi-Fi configuration write failed: \(error)")
        } else {
            wifiWriteMessage =
                "Bluetooth write acknowledged."
            
            if isWifiProvisioningPending {
                startWifiResultDeadline()
            }
        }
    }
    
    func sendWifiConfiguration(ssid: String, password: String) {
        guard canStartWifiConfiguration else {
            return
        }

        guard isConnected,
              let peripheral = vehiclePeripheral,
              let characteristic = wifiConfigCharacteristic else {
            wifiWriteMessage = "Wi-Fi configuration is unavailable."
            return
        }

        guard let credentials = WifiCredentials(
            ssid: ssid,
            password: password
        ) else {
            wifiWriteMessage = "Network details are invalid."
            return
        }

        let payload =
            "ssid=\(credentials.ssid),password=\(credentials.password)"
        let data = Data(payload.utf8)

        guard data.count <= peripheral.maximumWriteValueLength(
            for: .withResponse
        ) else {
            wifiWriteMessage = "Credentials exceed the Bluetooth write limit."
            return
        }

        wifiWriteMessage = nil
        isSendingWifiConfiguration = true
        isWifiProvisioningPending = true

        peripheral.writeValue(
            data,
            for: characteristic,
            type: .withResponse
        )
    }
    
    private func startConnectionDeadline(for peripheral: CBPeripheral) {
        cancelConnectionDeadline()

        connectionDeadlineTask = Task { [weak self] in
            do {
                try await Task.sleep(for: .seconds(15))
            } catch {
                return
            }

            guard let self,
                  self.centralManager.state == .poweredOn,
                  self.vehiclePeripheral === peripheral,
                  peripheral.state == .connecting else {
                return
            }

            print("BLE connection attempt timed out")
            self.centralManager.cancelPeripheralConnection(peripheral)
        }
    }

    private func cancelConnectionDeadline() {
        connectionDeadlineTask?.cancel()
        connectionDeadlineTask = nil
    }
    
    private func scheduleScanRetry() {
        cancelScanRetry()

        guard centralManager.state == .poweredOn,
              vehiclePeripheral == nil else {
            return
        }

        scanRetryTask = Task { [weak self] in
            do {
                try await Task.sleep(for: .seconds(2))
            } catch {
                return
            }

            guard let self,
                  self.centralManager.state == .poweredOn,
                  self.vehiclePeripheral == nil,
                  !self.centralManager.isScanning else {
                return
            }

            print("Retrying BLE scan")

            self.connectionPhase = .scanning
            self.centralManager.scanForPeripherals(
                withServices: [self.vehicleServiceUUID],
                options: nil
            )
        }
    }

    private func cancelScanRetry() {
        scanRetryTask?.cancel()
        scanRetryTask = nil
    }
    
    private func clearConnectionState() {
        cancelScanRetry()
        cancelConnectionDeadline()
        cancelWifiResultDeadline()
        connectionPhase = .idle
        canConfigureWifi = false
        hasReceivedTelemetry = false
        deviceIdentity = nil
                
        vehiclePeripheral = nil
        vehicleStateCharacteristic = nil
        wifiConfigCharacteristic = nil
        isSendingWifiConfiguration = false
        wifiWriteMessage = nil
        wifiProvisioningState = nil
        isWifiProvisioningPending = false
    }
    
    private func startWifiResultDeadline() {
        cancelWifiResultDeadline()
        
        wifiResultDeadlineTask = Task { [weak self] in
            do {
                try await Task.sleep(for: .seconds(25))
            } catch {
                return
            }
            
            guard let self,
                  self.isWifiProvisioningPending else {
                return
            }
            
            self.wifiWriteMessage = "No Wi-Fi result received. Reconnect Bluetooth before trying again."
        }
    }
    
    private func cancelWifiResultDeadline() {
        wifiResultDeadlineTask?.cancel()
        wifiResultDeadlineTask = nil
    }
}
