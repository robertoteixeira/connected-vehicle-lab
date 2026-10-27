//
//  DeviceIdentity.swift
//  ConnectedVehicle
//
//  Created by Roberto Teixeira on 21/10/2026.
//

struct DeviceIdentity: Equatable {
    let deviceId: String
    let hardwareVersion: String
    let firmwareVersion: String

    init?(blePayload: String) {
        var values: [String: String] = [:]

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

            guard values[key] == nil else {
                return nil
            }

            values[key] = String(parts[1])
        }

        guard
            let deviceId = values["id"],
            let hardwareVersion = values["hw"],
            let firmwareVersion = values["fw"]
        else {
            return nil
        }

        self.deviceId = deviceId
        self.hardwareVersion = hardwareVersion
        self.firmwareVersion = firmwareVersion
    }
}
