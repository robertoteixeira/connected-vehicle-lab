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
        VStack(spacing: 12) {
            Text("Connected Vehicle")
                .font(.title)

            Text("Speed: \(bluetoothManager.vehicleState.speedKph, specifier: "%.1f") km/h")
            Text("RPM: \(bluetoothManager.vehicleState.engineRpm, specifier: "%.0f")")
            Text("Throttle: \(bluetoothManager.vehicleState.throttlePosition, specifier: "%.1f")%")

            Text("Ignition: \(bluetoothManager.vehicleState.ignitionOn ? "ON" : "OFF")")
            Text("Brake: \(bluetoothManager.vehicleState.brakePressed ? "ON" : "OFF")")
            Text("Headlights: \(bluetoothManager.vehicleState.headlightsOn ? "ON" : "OFF")")
        }
        .padding()
    }
}

#Preview {
    ContentView()
}
