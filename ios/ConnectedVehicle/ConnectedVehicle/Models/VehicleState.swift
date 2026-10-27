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
        var value: [String: String] = [:]

        for component in blePayload.split(
            separator: ",",
            omittingEmptySubsequences: false
        ) {
            let parts = component.split(
                separator: "=",
                maxSplits: 1,
                omittingEmptySubsequences: false
            )

            guard parts.count == 2,
                  !parts[0].isEmpty,
                  !parts[1].isEmpty else {
                return nil
            }

            let key = String(parts[0])

            guard value[key] == nil else {
                return nil
            }

            value[key] = String(parts[1])
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
        
        ignitionOn = ignition == "1"
        brakePressed = brake == "1"
        headlightsOn = headlights == "1"
        
        self.speedKph = speedKph
        self.engineRpm = engineRpm
        self.throttlePosition = throttlePosition
    }
}
    
