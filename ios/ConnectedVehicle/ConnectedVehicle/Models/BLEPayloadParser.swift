//
//  BLEPayloadParser.swift
//  ConnectedVehicle
//
//  Created by Roberto Teixeira on 27/10/2026.
//

enum BLEPayloadParser {
    static func parse(_ payload: String) -> [String: String]? {
        var values: [String: String] = [:]

        for component in payload.split(
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

        return values
    }
}
