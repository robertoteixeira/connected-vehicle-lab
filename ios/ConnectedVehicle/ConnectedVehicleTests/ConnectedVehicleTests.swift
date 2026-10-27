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
    
    func testRejectsInvalidBooleanValues() {
        for field in ["ign", "brk", "lgt"] {
            for invalidValue in ["2", "-1", "true", "yes"] {
                let ignition = field == "ign" ? invalidValue : "1"
                let brake = field == "brk" ? invalidValue : "0"
                let headlights = field == "lgt" ? invalidValue : "1"

                let payload =
                    "ign=\(ignition),speed=42.5,rpm=2500," +
                    "thr=30.0,brk=\(brake),lgt=\(headlights)"

                XCTAssertNil(
                    VehicleState(blePayload: payload),
                    "Expected rejection for \(field)=\(invalidValue)"
                )
            }
        }
    }

    func testAcceptsAllBooleanValuesOff() throws {
        let state = try XCTUnwrap(
            VehicleState(
                blePayload: "ign=0,speed=0,rpm=0,thr=0,brk=0,lgt=0"
            )
        )

        XCTAssertFalse(state.ignitionOn)
        XCTAssertFalse(state.brakePressed)
        XCTAssertFalse(state.headlightsOn)
    }

    func testAcceptsAllBooleanValuesOn() throws {
        let state = try XCTUnwrap(
            VehicleState(
                blePayload: "ign=1,speed=0,rpm=800,thr=0,brk=1,lgt=1"
            )
        )

        XCTAssertTrue(state.ignitionOn)
        XCTAssertTrue(state.brakePressed)
        XCTAssertTrue(state.headlightsOn)
    }
}
