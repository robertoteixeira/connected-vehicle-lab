//
//  WifiProvisioningState.swift
//  ConnectedVehicle
//
//  Created by Roberto Teixeira on 30/10/2026.
//

enum WifiProvisioningState: String {
    case idle
    case connecting
    case connected
    case timedOut = "timed_out"
}
