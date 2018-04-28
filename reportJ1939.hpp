//
// Created by lianzeng on 18-4-25.
//

#ifndef OBD_REPORTJ1939_HPP
#define OBD_REPORTJ1939_HPP

struct J1939Reports //be used by IMPACT.
{
    float vehicleSpeed;     //km/h, VSpeed
    float engSpeed;         //rpm,  ESpeed
    int   engLoad;          //percent, ELoad
    float tripDistance;     //km,      TripDist
    float totalDistance;        //km   TotalDist
    int   engCoolanTemperature; //degC ECT
    int   fuelTemperature;      //degC FuelT
    int   airInleTemperature;   //degC AIT
    float electricalVoltage;    //V    EleV
    float batteryVoltage;       //V    BattV
    float engInletAirMfr;        //kg/h EIAMfr
    int   faultNum;              //     FaltNum
    float totalVehicleHours;    // Hour TVH
    float engTripFuelUsed;      //L     ETpFuel
    float engTotalFuelUsed;     // L    ETtFuel
    float engFuelRate;          // L/h  EFR
    int   parkingBrakeSwitch;//state[0,1,2,3],PBS
    int   brakeSwitch;      //state[0,1,2,3]  BS
};

#endif //OBD_REPORTJ1939_HPP
