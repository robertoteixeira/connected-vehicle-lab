//
//  ContentView.swift
//  ConnectedVehicle
//
//  Created by Roberto Teixeira on 18/10/2026.
//

import SwiftUI

struct ContentView: View {
    @EnvironmentObject private var bluetoothManager: BluetoothManager
    @State private var wifiSSID = ""
    @State private var wifiPassword = ""
    
    private var canSubmitWifi: Bool {
        bluetoothManager.canStartWifiConfiguration &&
        !wifiSSID.isEmpty &&
        wifiSSID.utf8.count <= 32 &&
        wifiPassword.utf8.count <= 64 &&
        !wifiSSID.contains(",password=")
    }
    
    var body: some View {
        ScrollView {
            VStack(spacing: 12) {
                Text("Connected Vehicle")
                    .font(.title)
                
                Label(
                    bluetoothManager.isConnected ? "Connected" : "Disconnected",
                    systemImage: bluetoothManager.isConnected
                    ? "checkmark.circle.fill"
                    : "exclamationmark.circle"
                )
                .foregroundStyle(bluetoothManager.isConnected ? Color.green : Color.secondary)
                
                if bluetoothManager.isConnected {
                    if let identity = bluetoothManager.deviceIdentity {
                        VStack(alignment: .leading, spacing: 6) {
                            Text("Device")
                                .font(.headline)
                            
                            LabeledContent("ID", value: identity.deviceId)
                            LabeledContent("Hardware", value: identity.hardwareVersion)
                            LabeledContent("Firmware ", value: identity.firmwareVersion)
                        }
                        .padding()
                        .background(
                            Color.secondary.opacity(0.08),
                            in: RoundedRectangle(cornerRadius: 12)
                        )
                    } else {
                        Text("Device identity unavailable")
                            .font(.footnote)
                            .foregroundStyle(.secondary)
                    }
                }
                
                if bluetoothManager.isConnected && bluetoothManager.hasReceivedTelemetry {
                    Text("Speed: \(bluetoothManager.vehicleState.speedKph, specifier: "%.1f") km/h")
                    Text("RPM: \(bluetoothManager.vehicleState.engineRpm, specifier: "%.0f")")
                    Text("Throttle: \(bluetoothManager.vehicleState.throttlePosition, specifier: "%.1f")%")
                    
                    Text("Ignition: \(bluetoothManager.vehicleState.ignitionOn ? "ON" : "OFF")")
                    Text("Brake: \(bluetoothManager.vehicleState.brakePressed ? "ON" : "OFF")")
                    Text("Headlights: \(bluetoothManager.vehicleState.headlightsOn ? "ON" : "OFF")")
                } else if bluetoothManager.isConnected {
                    ProgressView("Waiting for vehicle telemetry")
                } else {
                    Text("Vehicle telemetry is unavailable.")
                        .foregroundStyle(.secondary)
                }
                
                if bluetoothManager.isConnected {
                    Divider()
                    
                    VStack(alignment: .leading, spacing: 12) {
                        Text("Configure device Wi-Fi")
                            .font(.headline)
                        
                        TextField("Network name (SSID)", text: $wifiSSID)
                            .textInputAutocapitalization(.never)
                            .autocorrectionDisabled()
                            .textFieldStyle(.roundedBorder)
                        
                        SecureField("Password", text: $wifiPassword)
                            .textInputAutocapitalization(.never)
                            .autocorrectionDisabled()
                            .textFieldStyle(.roundedBorder)
                        
                        Button("Send Wi-Fi credentials") {
                            bluetoothManager.sendWifiConfiguration(
                                ssid: wifiSSID,
                                password: wifiPassword
                            )
                        }
                        .buttonStyle(.borderedProminent)
                        .disabled(!canSubmitWifi)
                        
                        if bluetoothManager.isSendingWifiConfiguration {
                            ProgressView("Sending credentials…")
                        }

                        if let message = bluetoothManager.wifiWriteMessage {
                            Text(message)
                                .font(.footnote)
                                .foregroundStyle(.secondary)
                        }
                        
                        VStack(alignment: .leading, spacing: 6) {
                            Text("Device Wi-Fi setup")
                                .font(.subheadline)
                                .bold()
                            
                            switch bluetoothManager.wifiProvisioningState {
                            case .idle:
                                Text("No connection attempt started.")
                                
                            case .connecting:
                                ProgressView("Connecting to Wi-Fi...")
                                
                            case .connected:
                                Label(
                                    "Last connection attempt succeeded",
                                    systemImage: "exclamationmark.triangle"
                                )
                                .foregroundStyle(.green)
                                
                            case .timedOut:
                                Label(
                                    "Connection timed out. Check the network details and try again.",
                                    systemImage: "exclamationmark.triangle"
                                )
                                .foregroundStyle(.orange)
                                
                            case nil:
                                Text("Wi-Fi setup status unavailable.")
                                    .foregroundStyle(.secondary)
                            }
                        }
                        .font(.footnote)
                    }
                }
            }
            .padding()
        }
    }
    

}

#Preview {
    ContentView()
}
