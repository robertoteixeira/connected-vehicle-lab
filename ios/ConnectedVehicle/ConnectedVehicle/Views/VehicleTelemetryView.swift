//
//  VehicleTelemetryView.swift
//  ConnectedVehicle
//
//  Created by Roberto Teixeira on 03/11/2026.
//

import SwiftUI

struct VehicleTelemetryView: View {
    let state: VehicleState

    var body: some View {
        VStack(spacing: 12) {
            Text("Speed: \(state.speedKph, specifier: "%.1f") km/h")
            Text("RPM: \(state.engineRpm, specifier: "%.0f")")
            Text("Throttle: \(state.throttlePosition, specifier: "%.1f")%")

            Text("Ignition: \(state.ignitionOn ? "ON" : "OFF")")
            Text("Brake: \(state.brakePressed ? "ON" : "OFF")")
            Text("Headlights: \(state.headlightsOn ? "ON" : "OFF")")
        }
    }
}

#Preview {
    VehicleTelemetryView(state: VehicleState())
}
