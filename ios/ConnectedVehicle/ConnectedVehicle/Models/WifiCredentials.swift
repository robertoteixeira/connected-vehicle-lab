//
//  WifiCredentials.swift
//  ConnectedVehicle
//
//  Created by Roberto Teixeira on 31/10/2026.
//

struct WifiCredentials {
    let ssid: String
    let password: String

    init?(ssid: String, password: String) {
        guard !ssid.isEmpty,
              ssid.utf8.count <= 32,
              password.utf8.count <= 64,
              !ssid.contains(",password="),
              !ssid.contains("\0"),
              !password.contains("\0") else {
            return nil
        }

        self.ssid = ssid
        self.password = password
    }
}
