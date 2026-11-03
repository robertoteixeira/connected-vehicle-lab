//
//  ContentView.swift
//  ConnectedVehicle
//
//  Created by Roberto Teixeira on 18/10/2026.
//

import SwiftUI

struct ContentView: View {
    @EnvironmentObject private var bluetoothManager: BluetoothManager
    
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
                    VehicleTelemetryView(state: bluetoothManager.vehicleState)
                } else if bluetoothManager.isConnected {
                    ProgressView("Waiting for vehicle telemetry")
                } else {
                    Text("Vehicle telemetry is unavailable.")
                        .foregroundStyle(.secondary)
                }
                
                if bluetoothManager.isConnected {
                    Divider()

                    WifiProvisioningView(bluetoothManager: bluetoothManager)
                }
            }
            .padding()
        }
    }
    

}

#Preview {
    ContentView()
}
