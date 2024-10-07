### 1. Installation de la Bibliothèque Paho MQTT (paho.mqtt.c et paho.mqtt.cpp)
#### Étape 1 : Cloner et Installer la Bibliothèque Paho MQTT C
La bibliothèque C est une dépendance requise pour la version C++. Voici comment l'installer :

1. **Installer les Dépendances Requises**
   Vous aurez besoin de `CMake` et de l'outil `build-essential` pour compiler la bibliothèque. Installez-les selon les spécificités de votre système.

   ```bash
   # Installer CMake (assurez-vous qu'il est disponible sur votre système)
   curl -O https://cmake.org/files/LatestRelease/cmake-<version>.tar.gz
   tar -zxvf cmake-<version>.tar.gz
   cd cmake-<version>
   ./bootstrap
   make
   sudo make install
   ```

2. **Cloner le Dépôt Paho MQTT C**
   ```bash
   git clone https://github.com/eclipse/paho.mqtt.c.git
   cd paho.mqtt.c
   ```

3. **Compiler et Installer**
   ```bash
   mkdir build && cd build
   cmake ..
   make
   sudo make install
   ```

#### Étape 2 : Cloner et Installer la Bibliothèque Paho MQTT C++
Une fois que la bibliothèque C est installée, vous pouvez installer la version C++.

1. **Cloner le Dépôt Paho MQTT C++**
   ```bash
   git clone https://github.com/eclipse/paho.mqtt.cpp.git
   cd paho.mqtt.cpp
   ```

2. **Compiler et Installer**
   ```bash
   mkdir build && cd build
   cmake -DPAHO_WITH_SSL=ON ..
   make
   sudo make install
   ```

### 2. Installation de la Bibliothèque JSON (nlohmann/json)
La bibliothèque `nlohmann/json` est une bibliothèque header-only. Cela signifie que vous n'avez pas besoin de la compiler ou de l'installer comme une bibliothèque partagée. Voici deux méthodes pour l'utiliser :

1. **Télécharger le Fichier d'En-tête**
   Vous pouvez télécharger le fichier unique `json.hpp` depuis le dépôt GitHub officiel.

   ```bash
   wget https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
   mkdir -p /usr/local/include/nlohmann/
   sudo mv json.hpp /usr/local/include/nlohmann/
   ```

2. **Inclure la Bibliothèque dans Votre Projet**
   Ajoutez l'en-tête directement dans votre projet. Par exemple, dans `main.cpp` :

   ```cpp
   #include <nlohmann/json.hpp>
   using json = nlohmann::json;
   ```

### 3. Compilation et Makefile
Voici une version mise à jour du `Makefile`, en tenant compte de l'installation manuelle.

#### Makefile

```makefile
CXX = g++
CXXFLAGS = -std=c++17 -I/usr/local/include
LDFLAGS = -L/usr/local/lib -lpaho-mqttpp3 -lpaho-mqtt3as

TARGET = mqtt_client
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
```

### Explication des Modifications
- **`CXXFLAGS`** : J'ai défini l'option `-I/usr/local/include` pour inclure les fichiers d'en-tête que nous avons installés dans `/usr/local/include`.
- **`LDFLAGS`** : J'ai ajouté l'option `-L/usr/local/lib` pour spécifier le chemin où les bibliothèques compilées (comme Paho) sont installées.
  
### Utilisation du Makefile
1. Créez un fichier `Makefile` dans le même dossier que votre fichier `main.cpp`.
2. Pour compiler le programme, exécutez la commande suivante dans le terminal :

   ```bash
   make
   ```

   Cela va produire un exécutable appelé `mqtt_client`.
3. Pour exécuter le programme, utilisez :

   ```bash
   ./mqtt_client
   ```

### Résumé des Étapes d'Installation
1. **CMake** : Installez CMake si ce n'est pas déjà fait.
2. **Paho MQTT C** :
   - Clonez, compilez et installez la bibliothèque C.
3. **Paho MQTT C++** :
   - Clonez, compilez et installez la bibliothèque C++.
4. **nlohmann/json** :
   - Téléchargez le fichier d'en-tête `json.hpp` et ajoutez-le à `/usr/local/include`.
5. **Compilation** :
   - Utilisez le `Makefile` pour compiler votre code.

Avec ces étapes, vous devriez être en mesure de compiler et exécuter votre programme MQTT en C++. N'oubliez pas de vérifier que les droits d'accès sont suffisants (certaines commandes peuvent nécessiter l'utilisation de `sudo`).




export DYLD_LIBRARY_PATH=/usr/local/lib:$DYLD_LIBRARY_PATH
./mqtt_client