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
    
    func testRejectsInvalidNumericValues() {
        let invalidValuesByField = [
            "speed": ["-1", "180.1", "nan", "inf", "-inf", "abc"],
            "rpm": ["-1", "6001", "nan", "inf", "-inf", "abc"],
            "thr": ["-1", "100.1", "nan", "inf", "-inf", "abc"]
        ]

        for (field, invalidValues) in invalidValuesByField {
            for invalidValue in invalidValues {
                let speed = field == "speed" ? invalidValue : "42.5"
                let rpm = field == "rpm" ? invalidValue : "2500"
                let throttle = field == "thr" ? invalidValue : "30.0"

                let payload =
                    "ign=1,speed=\(speed),rpm=\(rpm)," +
                    "thr=\(throttle),brk=0,lgt=1"

                XCTAssertNil(
                    VehicleState(blePayload: payload),
                    "Expected rejection for \(field)=\(invalidValue)"
                )
            }
        }
    }
    
    func testAcceptsNumericBoundaries() throws {
        for (speed, rpm, throttle) in [
            (0, 0, 0),
            (180, 6000, 100)
        ] {
            let payload =
                "ign=1,speed=\(speed),rpm=\(rpm)," +
                "thr=\(throttle),brk=0,lgt=1"

            let state = try XCTUnwrap(
                VehicleState(blePayload: payload)
            )

            XCTAssertEqual(state.speedKph, Float(speed))
            XCTAssertEqual(state.engineRpm, Float(rpm))
            XCTAssertEqual(state.throttlePosition, Float(throttle))
        }
    }
}
