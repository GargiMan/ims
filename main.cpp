/**
 * @file main.cpp
 * @author Marek Gergel (xgerge01)
 * @brief Solar power system simulation
 * @date 2024-11-25
 */

#include <simlib.h>
#include <math.h>
#include <iostream>

#define SECONDS_PER_DAY 86400
#define CURRENT_SECONDS_OF_DAY (((unsigned long)Time) % SECONDS_PER_DAY)

// System parameters

const unsigned long RECORD_SYSTEM_STATE_INTERVAL = 60; // seconds

const unsigned long POWER_CAP_GRID = 11000; // 3 * 16A * 230V
const unsigned long POWER_CAP_INVERTER = 7200; // 3 * 50A * 48V
const unsigned long POWER_CAP_BATTERY = 165e6; // 2 * 480Ah * 48V * 3600s

const double ENERGY_REQUIRED_MEAN = 600.0;
const double ENERGY_REQUIRED_DEVIATION = 1.0;

const double ENERGY_GENERATED_MAX = 8000.0; // W  
const double ENERGY_GENERATED_MEAN = 2000.0; // W  v zime 600W priemer za sec
const double ENERGY_GENERATED_DEVIATION = 0.7;

// Simulation parameters (default values)

enum Weather { SUNNY, CLOUDY, RAINY };
Weather currentWeather = SUNNY;

double transitionMatrix[3][3];

double batteryCharge = 0.5;
double cloudyProbabilityPercentage = 0.5;
double rainyProbabilityPercentage = 0.1;
unsigned long daysOfSimulation = 1;
unsigned long startOfSimulationSeconds = 1;
unsigned long sunriseSeconds = 0;
unsigned long sunsetSeconds = SECONDS_PER_DAY;

// Model components

Histogram RequiredEnergy("Required energy", 0.0, ((POWER_CAP_INVERTER + POWER_CAP_GRID)/20.0), 20);
Histogram GeneratedEnergy("Generated energy", 0.00001, (ENERGY_GENERATED_MAX/20.0), 20);
Histogram NotStoredEnergy("Not stored energy", 0.00001, (ENERGY_GENERATED_MAX/20.0), 20);

Histogram BatteryState("Battery state", 0.0, 0.10001, 10);
Histogram InverterState("Inverter state", 0.0, 0.10001, 10);
Histogram GridState("Grid state", 0.0, 0.10001, 10);

Store Battery("Battery", POWER_CAP_BATTERY);
Store Inverter("Inverter", POWER_CAP_INVERTER);
Store Grid("Grid", POWER_CAP_GRID);

// Model generators and events

Weather generateWeather(Weather current) {
    double p = Random();
    double cumulativeProbability = 0.0;
    for (int i = 0; i < 3; i++) {
        cumulativeProbability += transitionMatrix[current][i];
        if (p < cumulativeProbability) return static_cast<Weather>(i);
    }
    return current;
}

class RecordSystemState : public Event {
    void Behavior() {
        BatteryState((double)Battery.Free() / (double)Battery.Capacity());
        InverterState((double)Inverter.Free() / (double)Inverter.Capacity());
        GridState((double)Grid.Free() / (double)Grid.Capacity());
        Activate(Time + RECORD_SYSTEM_STATE_INTERVAL);
    }
};

class BatteryInit : public Process {
    void Behavior() {
        unsigned long missingBatteryCharge = POWER_CAP_BATTERY - (unsigned long)(((double)POWER_CAP_BATTERY) * (batteryCharge));
        //Print("BatteryInit %f\n", missingBatteryCharge);
        Enter(Battery, missingBatteryCharge);
    }
};

class PowerCharge : public Process {
    double generatedEnergy = 0;
    
    void Behavior() {
        // Weather change
        currentWeather = generateWeather(currentWeather);
        
        if (CURRENT_SECONDS_OF_DAY > sunriseSeconds && CURRENT_SECONDS_OF_DAY < sunsetSeconds) {
            generatedEnergy = ENERGY_GENERATED_MAX * Sin(M_PI * (CURRENT_SECONDS_OF_DAY - sunriseSeconds) / (sunsetSeconds - sunriseSeconds)).Value();

            // Weather dependent energy generation
            switch (currentWeather) {
                case SUNNY:
                    generatedEnergy *= Uniform(0.7, 1.0);
                    break;
                case CLOUDY:
                    generatedEnergy *= Uniform(0.3, 0.7);
                    break;
                case RAINY:
                    generatedEnergy *= Uniform(0.0, 0.3);
                    break;
                default:
                    break;
            }

            GeneratedEnergy((unsigned long)generatedEnergy);

            unsigned long chargedEnergy = Battery.Used() >= generatedEnergy ? generatedEnergy : Battery.Used();
            unsigned long notStoredEnergy = chargedEnergy == generatedEnergy ? 0 : generatedEnergy - chargedEnergy;
            if (notStoredEnergy > 0) {
                NotStoredEnergy(notStoredEnergy);
            }

            if (Battery.Used() != 0) {
                Leave(Battery, chargedEnergy);
                //Print("C %u\n", chargedEnergy);
            }
        }
    }
};

class PowerChargeGenerator : public Event {
    void Behavior() {
        (new PowerCharge)->Activate();
        Activate(Time + 1);
    }

public:
    PowerChargeGenerator() {
        (new BatteryInit)->Activate();
    }
};

class PowerRequired : public Process {
    unsigned long requiredEnergy;
    unsigned long inverterPowerCovered = 0;
    unsigned long gridPowerCovered = 0;

    void Behavior() {
        requiredEnergy = (unsigned long)(Exp(Normal(Ln(ENERGY_REQUIRED_MEAN).Value()-(Sqr(ENERGY_REQUIRED_DEVIATION).Value()/2.0), ENERGY_REQUIRED_DEVIATION)).Value());
        requiredEnergy = std::min(requiredEnergy, POWER_CAP_INVERTER + POWER_CAP_GRID); // limit required energy to the maximum combined power available
        RequiredEnergy(requiredEnergy);

        inverterPowerCovered = Inverter.Free() >= requiredEnergy ? requiredEnergy : Inverter.Free();
        
        // Power assist from grid
        if (inverterPowerCovered < requiredEnergy) {
            gridPowerCovered = Grid.Free() >= requiredEnergy - inverterPowerCovered ? requiredEnergy - inverterPowerCovered : Grid.Free();
        }

        // Inverter power consumption
        if (inverterPowerCovered > 0) {
            Enter(Battery, inverterPowerCovered);
            Enter(Inverter, inverterPowerCovered);
            Wait(1);
            Leave(Inverter, inverterPowerCovered);    
            //Print("I %u\t ", inverterPowerCovered);        
        } 
        
        // Grid power consumption
        if (gridPowerCovered > 0) {
            Enter(Grid, gridPowerCovered);
            Wait(1);
            Leave(Grid, gridPowerCovered);
            //Print("G %u\t ", gridPowerCovered);
        }

        //Print("\n");
    }
};

class PowerRequiredGenerator : public Event {
    void Behavior() {
        (new PowerRequired)->Activate();
        Activate(Time + 1);
    }
};

void parseArguments(int argc, char *argv[]) {
    if (argc != 8) {
        std::cerr << "Usage: " << argv[0] << " <daysOfSimulation> <startOfSimulationHour> <batteryChargePercentage> <sunriseHour> <sunsetHour> <cloudyProbabilityPercentage> <rainyProbabilityPercentage>" << std::endl;
        exit(1);
    }

    daysOfSimulation = atoi(argv[1]);
    if (daysOfSimulation <= 0) {
        std::cerr << "daysOfSimulation must be a positive integer" << std::endl;
        exit(1);
    }

    double startOfSimulationHour = atof(argv[2]);
    if (startOfSimulationHour < 0 || startOfSimulationHour > 24) {
        std::cerr << "startOfSimulationHour must be a number between 0 and 24" << std::endl;
        exit(1);
    }
    startOfSimulationSeconds = (unsigned long)(startOfSimulationHour * 3600.0);

    batteryCharge = atof(argv[3]);
    if (batteryCharge < 0 || batteryCharge > 100) {
        std::cerr << "batteryChargePercentage must be a number between 0 and 100" << std::endl;
        exit(1);
    }
    batteryCharge /= 100.0;

    double sunriseHour = atof(argv[4]);
    if (sunriseHour < 0 || sunriseHour > 24) {
        std::cerr << "sunriseHour must be a number between 0 and 24" << std::endl;
        exit(1);
    }
    sunriseSeconds = (unsigned long)(sunriseHour * 3600.0);
    
    double sunsetHour = atof(argv[5]);
    if (sunsetHour < 0 || sunsetHour > 24) {
        std::cerr << "sunsetHour must be a number between 0 and 24" << std::endl;
        exit(1);
    }
    sunsetSeconds = (unsigned long)(sunsetHour * 3600.0);

    cloudyProbabilityPercentage = atof(argv[6]);
    if (cloudyProbabilityPercentage < 0 || cloudyProbabilityPercentage > 100) {
        std::cerr << "cloudyProbabilityPercentage must be a number between 0 and 100" << std::endl;
        exit(1);
    }
    cloudyProbabilityPercentage /= 100.0;

    rainyProbabilityPercentage = atof(argv[7]);
    if (rainyProbabilityPercentage < 0 || rainyProbabilityPercentage > 100) {
        std::cerr << "rainyProbabilityPercentage must be a number between 0 and 100" << std::endl;
        exit(1);
    }
    rainyProbabilityPercentage /= 100.0;

    // Transition matrix
    transitionMatrix[SUNNY][SUNNY] = 1.0 - cloudyProbabilityPercentage;
    transitionMatrix[SUNNY][CLOUDY] = cloudyProbabilityPercentage / 3.0 * 2.0;
    transitionMatrix[SUNNY][RAINY] = cloudyProbabilityPercentage / 3.0 * 1.0;
    transitionMatrix[CLOUDY][SUNNY] = (1.0 - cloudyProbabilityPercentage) / 2.0;
    transitionMatrix[CLOUDY][CLOUDY] = cloudyProbabilityPercentage; 
    transitionMatrix[CLOUDY][RAINY] = (1.0 - cloudyProbabilityPercentage) / 2.0;
    transitionMatrix[RAINY][SUNNY] = (1.0 - rainyProbabilityPercentage) / 3.0 * 1.0;
    transitionMatrix[RAINY][CLOUDY] = (1.0 - rainyProbabilityPercentage) / 3.0 * 2.0;
    transitionMatrix[RAINY][RAINY] = rainyProbabilityPercentage;

}

int main(int argc, char *argv[]) {

    parseArguments(argc, argv);

    currentWeather = rainyProbabilityPercentage > Random() ? RAINY : (cloudyProbabilityPercentage > Random() ? CLOUDY : SUNNY);

    Init(startOfSimulationSeconds, startOfSimulationSeconds + (daysOfSimulation * SECONDS_PER_DAY));

    (new PowerChargeGenerator)->Activate();
    (new PowerRequiredGenerator)->Activate();
    (new RecordSystemState)->Activate();

    Run();

    Battery.Output();
    Inverter.Output();
    Grid.Output();
    
    BatteryState.Output();
    InverterState.Output();
    GridState.Output();

    RequiredEnergy.Output();
    GeneratedEnergy.Output();
    NotStoredEnergy.Output();

	return 0;
}