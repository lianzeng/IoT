//
// Created by lianzeng on 18-4-25.
//

#ifndef OBD_REPORTJ1939_HPP
#define OBD_REPORTJ1939_HPP

#define MAX_FAULT_NUM_REPORT   2
#define FAULT_LENGTH_15765     8   //eg:"P1012 "

struct ObdDataInfo //be used by IMPACT.
{
    char  state[10];        //running, stopped.
    float vehicleSpeed;     //km/h, VSpeed
    float engSpeed;         //rpm,  ESpeed
    int   engLoad;          //percent, ELoad
    float engRunTime;       //engine run time after ignite
    float engRunTimeWithFault; //unit hours
    float engRunTimeWithoutFault;
    float tripDistance;     //km,      TripDist
    float distanceWithoutFault; //distance without fault
    float distanceWithFault;  //distance with MIL fault
    float totalDistance;        //km   TotalDist
    int   engCoolanTemperature; //degC ECT
    int   fuelTemperature;      //degC FuelT
    int   airInleTemperature;   //degC AIT
    float electricalVoltage;    //V    EleV
    float batteryVoltage;       //V    BattV
    float engInletAirMfr;        //kg/h EIAMfr
    int   faultNum;              // DTC FaltNum
    char  falutDetail[MAX_FAULT_NUM_REPORT*FAULT_LENGTH_15765] ;        //eg:P1012, P1013
    float totalVehicleHours;    // Hour TVH
    float engTripFuelUsed;      //L     ETpFuel
    float engTotalFuelUsed;     // L    ETtFuel
    float engFuelRate;          // L/h  EFR
    float fuelLevelInput;       //percent of leftFuel/capacity
    int   parkingBrakeSwitch;//state[0,1,2,3],PBS
    int   brakeSwitch;      //state[0,1,2,3]  BS
    int   emergeBrake;      //emerge start or stop
    unsigned long timestamp;
};

#endif //OBD_REPORTJ1939_HPP
