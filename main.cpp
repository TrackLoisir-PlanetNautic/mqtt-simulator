#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>
#include <atomic>
#include <csignal>
#include <cstdlib>  // For std::rand() and std::srand()
#include <random>   // For random number generator

// Include Paho MQTT C++ headers
#include "mqtt/async_client.h"

// Include nlohmann/json for JSON handling
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// MQTT broker configuration
const std::string ADDRESS = "mqtt://51.83.47.206:1883";
const std::string CLIENT_ID = "mqtt_cpp_client";

std::atomic<bool> running(true);

// Declare serial_id as a global variable
std::string serial_id;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nInterruption signal received (Ctrl+C), stopping the program..." << std::endl;
        running = false;
    }
}

void publish_jetbrain_data(mqtt::async_client& client, int interval_seconds, const std::string& serial_id) {
    while (running) {
        try {
            while (!client.is_connected()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            json message;
            std::time_t t = std::time(nullptr);
            message["time"] = std::to_string(t);
            message["speed"] = std::rand() % 100;    // Random speed between 0 and 99
            message["battery"] = std::rand() % 100;  // Random battery between 0 and 99

            std::string payload = message.dump();
            std::string topic = "/data/" + serial_id + "/jetbrain";
            mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
            client.publish(pubmsg);

            std::cout << "[jetbrain] Message published on topic " << topic << ": " << payload << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
        } catch (const mqtt::exception& exc) {
            std::cerr << "[jetbrain] Error during publish: " << exc.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    std::cout << "[jetbrain] Thread stopped." << std::endl;
}

void publish_gps_data(mqtt::async_client& client, int interval_seconds, const std::string& serial_id) {
    // Configure a random number generator for latitude and longitude
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> lat_dist(44.0, 46.0);  // Latitude between 44 and 46
    std::uniform_real_distribution<> lon_dist(28.0, 30.0);  // Longitude between 28 and 30

    while (running) {
        try {
            while (!client.is_connected()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            json message;
            std::time_t t = std::time(nullptr);
            message["time"] = std::to_string(t);
            message["latitude"] = lat_dist(gen);
            message["longitude"] = lon_dist(gen);

            std::string payload = message.dump();
            std::string topic = "/data/" + serial_id + "/gps";
            mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
            client.publish(pubmsg);

            std::cout << "[GPS] Message published on topic " << topic << ": " << payload << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
        } catch (const mqtt::exception& exc) {
            std::cerr << "[GPS] Error during publish: " << exc.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    std::cout << "[GPS] Thread stopped." << std::endl;
}

void reconnect(mqtt::async_client& client, mqtt::connect_options& connOpts, const std::string& serial_id) {
    while (running) {
        try {
            std::cout << "Attempting to connect to the MQTT broker..." << std::endl;
            client.connect(connOpts)->wait();
            std::cout << "Successfully connected to the MQTT broker." << std::endl;

            // Publish a message indicating reconnection
            json message;
            message["status"] = "reconnected";
            message["time"] = std::to_string(std::time(nullptr));
            std::string payload = message.dump();
            std::string topic = "/reconnection/" + serial_id;
            mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
            client.publish(pubmsg);
            
            client.subscribe("/shadow/"+ serial_id + "/get/accepted/" , 1);
            // Request shadow update
            message["status"] = "need shadow";
            message["time"] = std::to_string(std::time(nullptr));
            payload = message.dump();
            topic = "/shadow/"+ serial_id + "/get";
            pubmsg = mqtt::make_message(topic, payload);
            client.publish(pubmsg);

            break;  // Exit the loop if the connection is successful
        } catch (const mqtt::exception& exc) {
            std::cerr << "Error during reconnection: " << exc.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void message_callback(mqtt::const_message_ptr msg) {
    std::cout << "Message received on topic: " << msg->get_topic() << "\nContent: " << msg->to_string() << std::endl;

    // Parse the JSON message and react based on the status
    try {
        json message = json::parse(msg->to_string());
        if (msg->get_topic().find("shadow/"+ serial_id + "/get/accepted/") != std::string::npos) {
            std::cout << "[SHADOW ACCEPTED] Update received: " << message.dump(4) << std::endl;
            // Process the shadow update as needed
        } else if (msg->get_topic().find("shadow/"+ serial_id + "/get/rejected/") != std::string::npos) {
            std::cout << "[SHADOW REJECTED] Request rejected: " << message.dump(4) << std::endl;
            // Handle errors or react to a rejection
        }
    } catch (const json::exception& e) {
        std::cerr << "Error parsing JSON message: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    mqtt::async_client client(ADDRESS, CLIENT_ID);
    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);
    std::cout << "Connecting to the MQTT broker " << ADDRESS << "..." << std::endl;

    // Handle interruption (Ctrl+C)
    std::signal(SIGINT, signal_handler);

    try {
        // Set the callback for message reception
        client.set_message_callback([&](mqtt::const_message_ptr msg) {
            message_callback(msg);
        });

        int jetbrain_interval = 5;  // Interval for jetbrain in seconds
        int gps_interval = 3;       // Interval for GPS in seconds

        // Generate a random serial ID
        std::srand(static_cast<unsigned int>(std::time(nullptr)));  // Seed the random number generator
        std::string serial_id = "GPS_" + std::to_string(std::rand() % 10000);

        // Start threads for sending messages
        std::thread jetbrain_thread(publish_jetbrain_data, std::ref(client), jetbrain_interval, serial_id);
        std::thread gps_thread(publish_gps_data, std::ref(client), gps_interval, serial_id);

        // Main loop to monitor the connection
        while (running) {
            if (!client.is_connected()) {
                reconnect(client, connOpts, serial_id);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Wait for threads to stop
        jetbrain_thread.join();
        gps_thread.join();

        // Disconnect from the MQTT broker
        client.disconnect()->wait();
        std::cout << "Disconnected from the MQTT broker." << std::endl;
    } catch (const mqtt::exception& exc) {
        std::cerr << "MQTT Error: " << exc.what() << std::endl;
        return 1;
    }

    std::cout << "Program stopped successfully." << std::endl;
    return 0;
}
