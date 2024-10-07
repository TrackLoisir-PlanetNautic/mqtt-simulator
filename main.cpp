#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>
#include <atomic>
#include <csignal>

// Inclure les en-têtes de Paho MQTT C++
#include "mqtt/async_client.h"

// Inclure la bibliothèque nlohmann/json pour JSON
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// Configuration du broker MQTT
const std::string ADDRESS = "mqtt://localhost:1883";
const std::string CLIENT_ID = "mqtt_cpp_client";

std::atomic<bool> running(true);

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nInterruption signalée (Ctrl+C), arrêt du programme..." << std::endl;
        running = false;
    }
}

void publish_torqeedo(mqtt::async_client& client, int interval_seconds) {
    while (running) {
        try {
            json message;
            std::time_t t = std::time(nullptr);
            message["time"] = std::to_string(t);
            message["speed"] = 20;
            message["Voltage"] = 30;

            std::string payload = message.dump();
            mqtt::message_ptr pubmsg = mqtt::make_message("/torqeedo", payload);
            client.publish(pubmsg);

            std::cout << "[TORQEEDO] Message publié: " << payload << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
        } catch (const mqtt::exception& exc) {
            std::cerr << "[TORQEEDO] Erreur lors de la publication: " << exc.what() << std::endl;
        }
    }
    std::cout << "[TORQEEDO] Thread désactivé." << std::endl;
}

void publish_gps(mqtt::async_client& client, int interval_seconds) {
    while (running) {
        try {
            json message;
            std::time_t t = std::time(nullptr);
            message["time"] = std::to_string(t);
            message["latitude"] = 45.2727; // Correction de 'lattitude' à 'latitude'
            message["longitude"] = 29.2882;

            std::string payload = message.dump();
            mqtt::message_ptr pubmsg = mqtt::make_message("/gps", payload);
            client.publish(pubmsg);

            std::cout << "[GPS] Message publié: " << payload << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
        } catch (const mqtt::exception& exc) {
            std::cerr << "[GPS] Erreur lors de la publication: " << exc.what() << std::endl;
        }
    }
    std::cout << "[GPS] Thread désactivé." << std::endl;
}

int main(int argc, char* argv[]) {
    mqtt::async_client client(ADDRESS, CLIENT_ID);
    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);
    std::cout << "Connexion au broker MQTT " << ADDRESS << "..." << std::endl;

    // Gestion de l'interruption (Ctrl+C)
    std::signal(SIGINT, signal_handler);

    try {
        // Connexion au broker MQTT
        client.connect(connOpts)->wait();
        std::cout << "Connecté au broker MQTT avec succès." << std::endl;

        int torqeedo_interval = 5; // Intervalle pour torqeedo en secondes
        int gps_interval = 3;      // Intervalle pour gps en secondes

        // Démarrer les threads pour l'envoi des messages
        std::thread torqeedo_thread(publish_torqeedo, std::ref(client), torqeedo_interval);
        std::thread gps_thread(publish_gps, std::ref(client), gps_interval);

        // Attendre l'arrêt du programme
        torqeedo_thread.join();
        gps_thread.join();

        // Déconnexion du broker MQTT
        client.disconnect()->wait();
        std::cout << "Déconnecté du broker MQTT." << std::endl;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "Erreur MQTT: " << exc.what() << std::endl;
        return 1;
    }

    std::cout << "Arrêt du programme avec succès." << std::endl;
    return 0;
}
