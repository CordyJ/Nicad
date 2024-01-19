//
//  REPerformanceTests.swift
//  REPerformanceTests
//
//  Created by Alan Yeung on 2017-04-24.
//  Copyright © 2017 Push Interactions, Inc. All rights reserved.
//

import XCTest
@testable import REPerformance

class REPerformanceTests: XCTestCase {

    let coordinator: AppCoordinator = AppCoordinator(appDelegate: FakeAppDelegate())

    override func setUp() {
        super.setUp()
        // Put setup code here. This method is called before the invocation of each test method in the class.
    }
    
    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
        super.tearDown()
    }
    
    func testExample() {
        // This is an example of a functional test case.
        // Use XCTAssert and related functions to verify your tests produce the correct results.
    }
    
    func testPerformanceExample() {
        // This is an example of a performance test case.
        self.measure {
            // Put the code you want to measure the time of here.
        }
    }
    
}

class FakeAppDelegate: NSObject, UIApplicationDelegate {

    var window: UIWindow? = UIWindow()

    

}
