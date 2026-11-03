//
//  WifiProvisioningView.swift
//  ConnectedVehicle
//
//  Created by Roberto Teixeira on 03/11/2026.
//

import SwiftUI

struct WifiProvisioningView: View {
    @ObservedObject var bluetoothManager: BluetoothManager

    @State private var wifiSSID = ""
    @State private var wifiPassword = ""

    private var canSubmitWifi: Bool {
        bluetoothManager.canStartWifiConfiguration &&
        WifiCredentials(
            ssid: wifiSSID,
            password: wifiPassword
        ) != nil
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Configure device Wi-Fi")
                .font(.headline)

            TextField("Network name (SSID)", text: $wifiSSID)
                .textInputAutocapitalization(.never)
                .autocorrectionDisabled()
                .textFieldStyle(.roundedBorder)

            SecureField("Password", text: $wifiPassword)
                .textInputAutocapitalization(.never)
                .autocorrectionDisabled()
                .textFieldStyle(.roundedBorder)

            Button("Send Wi-Fi credentials") {
                bluetoothManager.sendWifiConfiguration(
                    ssid: wifiSSID,
                    password: wifiPassword
                )
            }
            .buttonStyle(.borderedProminent)
            .disabled(!canSubmitWifi)

            if bluetoothManager.isSendingWifiConfiguration {
                ProgressView("Sending credentials…")
            }

            if let message = bluetoothManager.wifiWriteMessage {
                Text(message)
                    .font(.footnote)
                    .foregroundStyle(.secondary)
            }

            VStack(alignment: .leading, spacing: 6) {
                Text("Device Wi-Fi setup")
                    .font(.subheadline)
                    .bold()

                switch bluetoothManager.wifiProvisioningState {
                case .idle:
                    Text("No connection attempt started.")

                case .connecting:
                    ProgressView("Connecting to Wi-Fi…")

                case .connected:
                    Label(
                        "Last connection attempt succeeded",
                        systemImage: "checkmark.circle.fill"
                    )
                    .foregroundStyle(.green)

                case .timedOut:
                    Label(
                        "Connection timed out. Check the network details and try again.",
                        systemImage: "exclamationmark.triangle"
                    )
                    .foregroundStyle(.orange)

                case nil:
                    Text("Wi-Fi setup status unavailable.")
                        .foregroundStyle(.secondary)
                }
            }
            .font(.footnote)
        }
    }
}
