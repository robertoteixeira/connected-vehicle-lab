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
    
    init() {}
    
    init?(blePayload: String) {
        guard let value = BLEPayloadParser.parse(blePayload) else {
            return nil
        }
        
        guard
            let ignition = value["ign"],
            let speed = value["speed"],
            let rpm = value["rpm"],
            let throttle = value["thr"],
            let brake = value["brk"],
            let headlights = value["lgt"],
            let speedKph = Float(speed),
            let engineRpm = Float(rpm),
            let throttlePosition = Float(throttle)
        else { return nil }
        
        guard ignition == "0" || ignition == "1",
              brake == "0" || brake == "1",
              headlights == "0" || headlights == "1" else {
            return nil
        }
        
        guard speedKph.isFinite,
              engineRpm.isFinite,
              throttlePosition.isFinite,
              (0...180).contains(speedKph),
              (0...6000).contains(engineRpm),
              (0...100).contains(throttlePosition) else {
            return nil
        }
        
        ignitionOn = ignition == "1"
        brakePressed = brake == "1"
        headlightsOn = headlights == "1"
        
        self.speedKph = speedKph
        self.engineRpm = engineRpm
        self.throttlePosition = throttlePosition
    }
}
    
