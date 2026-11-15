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

  private enum Field: Hashable {
    case ssid
    case password
  }

  @FocusState private var focusedField: Field?

  private var canSubmitWifi: Bool {
    bluetoothManager.canStartWifiConfiguration
      && WifiCredentials(
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
        .focused($focusedField, equals: .ssid)

      SecureField("Password", text: $wifiPassword)
        .textInputAutocapitalization(.never)
        .autocorrectionDisabled()
        .textFieldStyle(.roundedBorder)
        .focused($focusedField, equals: .password)

      Button("Send Wi-Fi credentials") {
        if bluetoothManager.sendWifiConfiguration(
          ssid: wifiSSID,
          password: wifiPassword
        ) {
          wifiPassword = ""
          focusedField = nil
        }
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
            "Last Wi-Fi connection attempt timed out.",
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
