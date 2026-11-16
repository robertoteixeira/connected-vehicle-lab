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

	private var hasValidWifiCredentials: Bool {
		WifiCredentials(ssid: wifiSSID, password: wifiPassword) != nil
	}

	private var canSubmitWifi: Bool {
		bluetoothManager.canStartWifiConfiguration && hasValidWifiCredentials
	}

	private func submitWifiCredentials() {
		guard canSubmitWifi else {
			return
		}

		if bluetoothManager.sendWifiConfiguration(
			ssid: wifiSSID,
			password: wifiPassword
		) {
			wifiPassword = ""
			focusedField = nil
		}
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
				.submitLabel(.next)
				.onSubmit {
					focusedField = .password
				}

			SecureField("Password", text: $wifiPassword)
				.textInputAutocapitalization(.never)
				.autocorrectionDisabled()
				.textFieldStyle(.roundedBorder)
				.focused($focusedField, equals: .password)
				.submitLabel(.go)
				.onSubmit {
					submitWifiCredentials()
				}

			if (!wifiSSID.isEmpty || !wifiPassword.isEmpty) && !hasValidWifiCredentials
			{
				Text(
					"Enter a network name of 1–32 UTF-8 bytes and a password of up to 64 bytes."
				)
				.font(.footnote)
				.foregroundStyle(.secondary)
			}

			Button("Send Wi-Fi credentials", action: submitWifiCredentials)
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
