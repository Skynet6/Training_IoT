//
//
//

#include "MainApp.h"
#include <stdio.h>
#include <algorithm>

#include "Utils/TimeUtil.h"

#include "Feature/FeatureController.h"
#include "Feature/SwitchFeatureController.h"
#include "Feature/LedFeatureController.h"
#include "Feature/DHT22FeatureController.h"
#include "Feature/MotionSensorFeatureController.h"

// update with a unique id
#define DEVICE_UNIQUE_ID	"my_device_id"

// update with you network SSID and password
#define NETWORK_NAME			"IoT_Network"
#define NETWORK_PASSWORD	"IoT_Password"

// update with the host for the 06_RemoteControl_App ASP.NET Web App
#define SERVER_HOST				"http://iot-remotecontrol-2.azurewebsites.net/api/device/"

#define TOPIC_REGISTER		"register"
#define TOPIC_SENSOR			"sensor"

MainApp::MainApp()
	: _deviceConfig(DEVICE_UNIQUE_ID, NETWORK_NAME, NETWORK_PASSWORD),
		_serializationProvider(),
		_messageBus(SERVER_HOST, &_serializationProvider, this)
{
	// sub to topic of the device's unique id
	_messageBus.Subscribe(_deviceConfig.uniqueId);

	// define all the features
	_features.push_back(new SwitchFeatureController(4, this, 4, false));
	_features.push_back(new SwitchFeatureController(5, this, 5, false));

	_features.push_back(new LedFeatureController(1, this, 13));
	//_features.push_back(new LedFeatureController(2, this, 12));
	//_features.push_back(new LedFeatureController(3, this, 14));

	_features.push_back(new DHT22FeatureController(6, this, 12, TOPIC_SENSOR));
	_features.push_back(new MotionSensorFeatureController(7, this, 14, TOPIC_SENSOR));
}

MainApp::~MainApp()
{
	// destroy all the features
	std::for_each(_features.begin(), _features.end(), [](FeatureController* feature) {
		delete feature;
	});
	_features.clear();
}

void MainApp::Init()
{
	Serial.begin(115200);

	SetupWifi();
	OnStart();
}

void MainApp::Loop()
{
	_messageBus.Loop();
	OnLoop();
}

void MainApp::SetupWifi()
{
	Serial.printf("\n[MainApp] Connecting to %s\n", _deviceConfig.networkName);

  // Connect to a WiFi network
  WiFi.begin(_deviceConfig.networkName, _deviceConfig.networkPassword);
  // Wait until the connection is established
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.print("\n[MainApp] WiFi connected\n");
  Serial.print("[MainApp] IP: ");
  Serial.println(WiFi.localIP());
}

void MainApp::Handle(const char* topic, JsonObject& message)
{
	// if topic is same as device id
	if (strcmp(_deviceConfig.uniqueId, topic) == 0)
	{
		OnCommand(message);
	}
}

void MainApp::OnStart() {
	Serial.println("[MainApp] Starting...");

	SendDeviceDescription();

	Serial.println("[MainApp] Started.");
}

void MainApp::OnStop() {
	Serial.println("[MainApp] Stopping...");

	Serial.println("[MainApp] Stopped.");
}

void MainApp::OnLoop()
{
	// run loop on each feature instance (the new C++11 lambda syntax)
	std::for_each(_features.begin(), _features.end(), [](FeatureController* feature) {
		feature->Loop();
	});
	// for (auto featureIt = _features.begin(); featureIt != _features.end(); ++featureIt)
	//	(*featureIt)->Loop();
}

void MainApp::SendDeviceDescription()
{
		Serial.println("Sending DeviceDescription...");

		JsonObject& descriptionEvent = _serializationProvider.CreateMessage();
		descriptionEvent["deviceId"] = _deviceConfig.uniqueId;
		JsonArray& featureDescriptions = descriptionEvent.createNestedArray("features");

		std::for_each(_features.begin(), _features.end(), [&featureDescriptions](FeatureController* feature) {
			feature->PopulateDescriptions(featureDescriptions);
		});

		_messageBus.Publish(TOPIC_REGISTER, descriptionEvent);
}

void MainApp::OnCommand(JsonObject& command)
{
	Serial.println("[MainApp] OnCommand (start)");

	std::for_each(_features.begin(), _features.end(), [&command](FeatureController* feature) {
		feature->TryHandle(command);
	});

	Serial.println("[MainApp] OnCommand (finish)");
}
