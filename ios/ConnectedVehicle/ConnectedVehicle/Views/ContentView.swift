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
        VStack {
            Text("Speed: \(bluetoothManager.vehicleState.speedKph, specifier: "%.1f") km/h")
        }
        .padding()
    }
}

#Preview {
    ContentView()
}
