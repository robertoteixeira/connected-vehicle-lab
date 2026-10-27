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
        guard let values = BLEPayloadParser.parse(blePayload) else {
            return nil
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
