//
//  ConnectedVehicleTests.swift
//  ConnectedVehicleTests
//
//  Created by Roberto Teixeira on 18/10/2026.
//

import XCTest
@testable import ConnectedVehicle

final class ConnectedVehicleTests: XCTestCase {
    private let validPayload =
        "ign=1,speed=42.5,rpm=2500,thr=30.0,brk=0,lgt=1"

    func testParsesValidVehicleState() throws {
        let state = try XCTUnwrap(
            VehicleState(blePayload: validPayload)
        )

        XCTAssertTrue(state.ignitionOn)
        XCTAssertEqual(state.speedKph, 42.5)
        XCTAssertEqual(state.engineRpm, 2500)
        XCTAssertEqual(state.throttlePosition, 30)
        XCTAssertFalse(state.brakePressed)
        XCTAssertTrue(state.headlightsOn)
    }

    func testRejectsDuplicateFields() {
        XCTAssertNil(
            VehicleState(blePayload: validPayload + ",speed=99")
        )
    }

    func testRejectsMalformedFields() {
        for suffix in [",broken", ",", ",=value", ",extra="] {
            XCTAssertNil(
                VehicleState(blePayload: validPayload + suffix),
                "Expected rejection for suffix: \(suffix)"
            )
        }
    }

    func testRejectsMissingRequiredField() {
        XCTAssertNil(
            VehicleState(
                blePayload: "ign=1,speed=42.5,rpm=2500,thr=30.0,brk=0"
            )
        )
    }
}
