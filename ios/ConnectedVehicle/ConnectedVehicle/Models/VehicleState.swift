//
//  VehicleState.swift
//  ConnectedVehicle
//
//  Created by Roberto Teixeira on 18/10/2026.
//

struct VehicleState: Equatable {
    var ignitionOn = false
    var brakePressed = false
    var headlightsOn = false

    var speedKph: Float = 0
    var engineRpm: Float = 0
    var throttlePosition: Float = 0
}
