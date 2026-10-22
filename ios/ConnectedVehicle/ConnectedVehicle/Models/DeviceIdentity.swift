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
        let values = Dictionary(
            uniqueKeysWithValues: blePayload
                .split(separator: ",")
                .compactMap { component -> (String, String)? in
                    let parts = component.split(separator: "=", maxSplits: 1)

                    guard parts.count == 2 else {
                        return nil
                    }

                    return (String(parts[0]), String(parts[1]))
                }
        )

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
