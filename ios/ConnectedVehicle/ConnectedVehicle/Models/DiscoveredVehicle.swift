//
//  DiscoveredVehicle.swift
//  ConnectedVehicle
//
//  Created by Roberto Teixeira on 08/11/2026.
//

import CoreBluetooth
import Foundation

struct DiscoveredVehicle: Identifiable {
    let peripheral: CBPeripheral
    let rssi: Int
    
    var id: UUID {
        peripheral.identifier
    }
    
    var name: String {
        peripheral.name ?? "Unnamed vehicle"
    }
}
