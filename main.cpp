#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>
#include <atomic>
#include <csignal>
#include <cstdlib>  // Pour std::rand() et std::srand()
#include <random>   // Pour un générateur de nombres aléatoires

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

void publish_torqeedo(mqtt::async_client& client, int interval_seconds, const std::string& serial_id) {
    while (running) {
        try {
            while (!client.is_connected()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            json message;
            std::time_t t = std::time(nullptr);
            message["time"] = std::to_string(t);
            message["reported"] = {};
            message["reported"]["speed"] = std::rand() % 100;  // Vitesse aléatoire entre 0 et 100
            message["reported"]["battery"] = std::rand() % 100; // Batterie aléatoire entre 0 et 100

            std::string payload = message.dump();
            std::string topic = "/update/" + serial_id + "/torqeedo";
            mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
            client.publish(pubmsg);

            std::cout << "[TORQEEDO] Message publié sur le topic " << topic << ": " << payload << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
        } catch (const mqtt::exception& exc) {
            std::cerr << "[TORQEEDO] Erreur lors de la publication: " << exc.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    std::cout << "[TORQEEDO] Thread désactivé." << std::endl;
}

void publish_gps(mqtt::async_client& client, int interval_seconds, const std::string& serial_id) {
    // Configurer un générateur de nombres aléatoires pour latitude et longitude
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> lat_dist(44.0, 46.0);  // Latitude entre 44 et 46
    std::uniform_real_distribution<> lon_dist(28.0, 30.0);  // Longitude entre 28 et 30

    while (running) {
        try {
            while (!client.is_connected()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            json message;
            std::time_t t = std::time(nullptr);
            message["time"] = std::to_string(t);
            message["reported"] = {};
            message["reported"]["latitude"] = lat_dist(gen);
            message["reported"]["longitude"] = lon_dist(gen);

            std::string payload = message.dump();
            std::string topic = "/update/" + serial_id + "/gps";
            mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
            client.publish(pubmsg);

            std::cout << "[GPS] Message publié sur le topic " << topic << ": " << payload << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
        } catch (const mqtt::exception& exc) {
            std::cerr << "[GPS] Erreur lors de la publication: " << exc.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    std::cout << "[GPS] Thread désactivé." << std::endl;
}

void reconnect(mqtt::async_client& client, mqtt::connect_options& connOpts, const std::string& serial_id) {
    while (running) {
        try {
            std::cout << "Tentative de connexion au broker MQTT..." << std::endl;
            client.connect(connOpts)->wait();
            std::cout << "Connecté au broker MQTT avec succès." << std::endl;

            // Publier un message spécifique indiquant la reconnexion
            json message;
            message["status"] = "reconnected";
            message["time"] = std::to_string(std::time(nullptr));
            std::string payload = message.dump();
            std::string topic = "/reconnected/" + serial_id;
            mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
            client.publish(pubmsg);
            message["status"] = "need shadow";
            message["time"] = std::to_string(std::time(nullptr));
            payload = message.dump();
            topic = "/get/" + serial_id;
            pubmsg = mqtt::make_message(topic, payload);
            client.publish(pubmsg);

            break; // Quitter la boucle si la connexion réussit
        } catch (const mqtt::exception& exc) {
            std::cerr << "Erreur lors de la reconnexion: " << exc.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void message_callback(mqtt::const_message_ptr msg) {
    std::cout << "Message reçu sur le topic: " << msg->get_topic() << "\nContenu: " << msg->to_string() << std::endl;

    // Analyser le message JSON et réagir en fonction du statut
    try {
        json message = json::parse(msg->to_string());
        if (msg->get_topic().find("/get/accepted/") != std::string::npos) {
            std::cout << "[SHADOW ACCEPTED] Mise à jour reçue: " << message.dump(4) << std::endl;
            // Ici, vous pouvez traiter la mise à jour du shadow comme nécessaire
        } else if (msg->get_topic().find("/get/rejected/") != std::string::npos) {
            std::cout << "[SHADOW REJECTED] Demande rejetée: " << message.dump(4) << std::endl;
            // Ici, vous pouvez traiter les erreurs ou réagir à un rejet
        }
    } catch (const json::exception& e) {
        std::cerr << "Erreur lors de l'analyse du message JSON: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    mqtt::async_client client(ADDRESS, CLIENT_ID);
    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);
    std::cout << "Connexion au broker MQTT " << ADDRESS << "..." << std::endl;

    // Gestion de l'interruption (Ctrl+C)
    std::signal(SIGINT, signal_handler);

    try {
        // Configurer le callback pour la réception de messages
        client.set_callback([&](mqtt::const_message_ptr msg) {
            message_callback(msg);
        });

        int torqeedo_interval = 5; // Intervalle pour torqeedo en secondes
        int gps_interval = 3;      // Intervalle pour gps en secondes

        // Générer un identifiant de série aléatoire pour GPS
        std::string serial_id = "GPS_" + std::to_string(std::rand() % 10000);

        // Démarrer les threads pour l'envoi des messages
        std::thread torqeedo_thread(publish_torqeedo, std::ref(client), torqeedo_interval, serial_id);
        std::thread gps_thread(publish_gps, std::ref(client), gps_interval, serial_id);

        // Boucle principale pour surveiller la connexion
        while (running) {
            if (!client.is_connected()) {
                reconnect(client, connOpts, serial_id);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Attendre l'arrêt des threads
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