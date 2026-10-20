//
//  ConnectedVehicleApp.swift
//  ConnectedVehicle
//
//  Created by Roberto Teixeira on 18/10/2026.
//

import SwiftUI

@main
struct ConnectedVehicleApp: App {
    @StateObject private var bluetoothManager = BluetoothManager()
    
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}
