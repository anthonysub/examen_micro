#include <WiFi.h>
#include <WebServer.h>

// Credenciales WiFi
const char* ssid = "TIGO-A10";
const char* password = "12345678";

WebServer server(80);

// Pin del potenciómetro
const int pinpotenciometro = 34; 

//Leds
int ledPins[] = {2, 4, 16};
const int pinCount = 3;

// Variables de control
int valor = 0;          
int tiempoEspera = 0;  
int umbral = 1000; // Variable global para el umbral personalizable

// Función principal
void encender() {

  valor = analogRead(pinpotenciometro);

  // Verificación dinámica usando la variable 'umbral'
  if (valor > umbral) {
      
      // Mapeo dinámico: inicia en el umbral actual y termina en 4095
      int maxVal = 4095;
      if (umbral >= maxVal) maxVal = umbral + 1; // Seguridad matemática
      
      tiempoEspera = map(valor, umbral, maxVal, 500, 50);
      tiempoEspera = constrain(tiempoEspera, 50, 500);

      // Encender los 3 LEDs a la vez
      for (int i = 0; i < pinCount; i++) {
        digitalWrite(ledPins[i], HIGH); 
      }
      delay(tiempoEspera);        
  
      // Apagar los 3 LEDs a la vez
      for (int i = 0; i < pinCount; i++) {
        digitalWrite(ledPins[i], LOW);  
      }
      delay(tiempoEspera);        
      
  } else {
      for (int i = 0; i < pinCount; i++) {
        digitalWrite(ledPins[i], LOW); 
      }
      tiempoEspera = 0; 
  }
} // <-- Llave de cierre corregida

// Interfaz Web 
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<title>Control LEDs ESP32</title>";
  html += "<style>";
  html += "body { font-family: Arial; text-align:center; margin-top:50px;}";
  html += ".circle { display: flex; justify-content: center; align-items: center; font-size: 15px; margin: 10px auto; padding: 10px; border-radius: 50%; width: 130px; height: 130px; text-align: center; }";
  html += ".on { background-color: #d4edda; color: #155724; font-weight: bold; border: 1px solid #c3e6cb; }";
  html += ".off { background-color: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }";
  html += "input { padding: 8px; font-size: 16px; width: 150px; text-align: center; }";
  html += "button { padding: 8px 15px; font-size: 16px; cursor: pointer; }";
  html += "</style>";
  html += "</head><body>";
  
  html += "<h1>Monitoreo de Umbral y Velocidad</h1>";

  // Panel de control del umbral
  html += "<div style='margin-bottom: 25px; padding: 15px; background: #f4f4f4; border-radius: 8px; display: inline-block;'>";
  html += "  <p>Umbral actual: <strong id='umbral-actual'>...</strong></p>";
  html += "  <input type='number' id='inputUmbral' placeholder='Ej. 2000' min='0' max='4095'>";
  html += "  <button onclick='actualizarUmbral()'>Fijar Umbral</button>";
  html += "</div>";

  html += "<div id='estado-container' class='circle off'>Cargando...</div>";

  // JavaScript 
  html += "<script>";
  
  // Función para enviar el nuevo umbral al ESP32
  html += "function actualizarUmbral() {";
  html += "  const nuevoValor = document.getElementById('inputUmbral').value;";
  html += "  if(nuevoValor !== '') {";
  html += "    fetch('/set_umbral?val=' + nuevoValor);";
  html += "  }";
  html += "}";

  // Función de polling para actualizar UI
  html += "function actualizarDatos() {";
  html += "  fetch('/datos')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      document.getElementById('umbral-actual').innerText = data.umbral;"; // Actualiza la UI con el umbral real
  html += "      const contenedor = document.getElementById('estado-container');";
  html += "      if (data.valor > data.umbral) {"; // Compara con el umbral dinámico del JSON
  html += "        contenedor.className = 'circle on';";
  html += "        contenedor.innerHTML = '<b>¡Activo!<br>Lectura: ' + data.valor + '<br><span style=\"font-size:12px\">Parpadeo: ' + data.velocidad + 'ms</span></b>';";
  html += "      } else {";
  html += "        contenedor.className = 'circle off';";
  html += "        contenedor.innerHTML = '<b>Apagado<br>Lectura: ' + data.valor + '</b>';";
  html += "      }";
  html += "    });";
  html += "}";
  html += "setInterval(actualizarDatos, 500);";
  html += "actualizarDatos();";
  html += "</script>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

// Endpoint JSON 
void handleDatos() {
  // Se agregó 'umbral' a la respuesta JSON
  String json = "{\"valor\":" + String(valor) + 
                ", \"velocidad\":" + String(tiempoEspera) + 
                ", \"umbral\":" + String(umbral) + "}";
  server.send(200, "application/json", json);
}

// Nuevo Endpoint para recibir la actualización del umbral
void handleSetUmbral() {
  if (server.hasArg("val")) {
    umbral = server.arg("val").toInt();
    umbral = constrain(umbral, 0, 4095); // Evita valores fuera de rango
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  // Configurar pines de los LEDs como salida
  for (int i = 0; i < pinCount; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  // Configurar  potenciómetro 
  pinMode(pinpotenciometro, INPUT);

  // Iniciar WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP de la ESP32: ");
  Serial.println(WiFi.localIP());

  // Rutas del servidor
  server.on("/", handleRoot);
  server.on("/datos", handleDatos);
  server.on("/set_umbral", handleSetUmbral); // Se registra la nueva ruta

  // iniciar servidor web
  server.begin();
  Serial.println("Servidor web iniciado");
}

void loop() {
  server.handleClient(); 
  encender();          
}
