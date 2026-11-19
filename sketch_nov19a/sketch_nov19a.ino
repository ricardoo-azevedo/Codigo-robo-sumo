
/*
 * ============================================
 * ROBO SUMO - UNINASSAU
 * Arduino UNO R3
 * ============================================
 *
 * Componentes:
 * - Arduino UNO R3
 * - TB6612FNG
 * - 2 motores TT
 * - 2 sensores TCRT5000
 * - HC-SR04
 * - Push Button
 *
 */

// ============================================
// 1. PINOS
// ============================================

// Ponte H - TB6612FNG
const int PWMA = 5;
const int AIN1 = 7;
const int AIN2 = 8;

const int PWMB = 6;
const int BIN1 = 9;
const int BIN2 = 10;

const int STBY = 4;

// Sensores TCRT5000
const int SENSOR_ESQUERDO = A0;
const int SENSOR_DIREITO = A1;

// Sensor HC-SR04
const int TRIG = 12;
const int ECHO = 11;

// Botao
const int BOTAO = 2;


// ============================================
// 2. CONFIGURACOES
// ============================================

// Velocidades dos motores
const int VELOCIDADE_ATAQUE = 220;
const int VELOCIDADE_BUSCA = 150;
const int VELOCIDADE_RECUO = 180;

// Distancia para detectar adversario (cm)
const int DISTANCIA_ATAQUE = 35;

// Tempo de espera antes da luta
const unsigned long TEMPO_INICIO = 5000;

// Tempo de movimentacao para escapar
const unsigned long TEMPO_RECUO = 300;
const unsigned long TEMPO_GIRO = 350;

// Ajustar conforme os sensores utilizados
// LOW = borda detectada
const int BORDA_DETECTADA = LOW;


// ============================================
// 3. CONTROLE DOS MOTORES
// ============================================

void motorEsquerdo(int velocidade) {

  velocidade = constrain(velocidade, -255, 255);

  if (velocidade > 0) {
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
  }
  else if (velocidade < 0) {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
  }
  else {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
  }

  analogWrite(PWMA, abs(velocidade));
}


void motorDireito(int velocidade) {

  velocidade = constrain(velocidade, -255, 255);

  if (velocidade > 0) {
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
  }
  else if (velocidade < 0) {
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
  }
  else {
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, LOW);
  }

  analogWrite(PWMB, abs(velocidade));
}


// Valores positivos = frente
// Valores negativos = tras

void movimentar(int esquerdo, int direito) {

  motorEsquerdo(esquerdo);
  motorDireito(direito);
}


void parar() {
  movimentar(0, 0);
}


void avancar(int velocidade) {
  movimentar(velocidade, velocidade);
}


void recuar(int velocidade) {
  movimentar(-velocidade, -velocidade);
}


void girarDireita(int velocidade) {
  movimentar(velocidade, -velocidade);
}


void girarEsquerda(int velocidade) {
  movimentar(-velocidade, velocidade);
}


// ============================================
// 4. SENSOR ULTRASSONICO HC-SR04
// ============================================

float medirDistancia() {

  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG, LOW);

  long duracao = pulseIn(ECHO, HIGH, 20000);

  // Sem retorno do sensor
  if (duracao == 0) {
    return -1;
  }

  // Conversao para centimetros
  float distancia = duracao * 0.0343 / 2;

  return distancia;
}


// ============================================
// 5. SENSORES DE BORDA TCRT5000
// ============================================

bool detectarBordaEsquerda() {

  return digitalRead(SENSOR_ESQUERDO)
         == BORDA_DETECTADA;
}


bool detectarBordaDireita() {

  return digitalRead(SENSOR_DIREITO)
         == BORDA_DETECTADA;
}


// ============================================
// 6. ESCAPAR DA BORDA
// ============================================

void escaparBorda() {

  bool esquerda = detectarBordaEsquerda();
  bool direita = detectarBordaDireita();

  parar();
  delay(50);

  // Recua para afastar o robo da borda
  recuar(VELOCIDADE_RECUO);
  delay(TEMPO_RECUO);

  parar();
  delay(50);

  // Os dois sensores encontraram a borda
  if (esquerda && direita) {

    girarDireita(VELOCIDADE_BUSCA);
    delay(500);

  }

  // Apenas o sensor esquerdo encontrou a borda
  else if (esquerda) {

    girarDireita(VELOCIDADE_BUSCA);
    delay(TEMPO_GIRO);

  }

  // Apenas o sensor direito encontrou a borda
  else if (direita) {

    girarEsquerda(VELOCIDADE_BUSCA);
    delay(TEMPO_GIRO);

  }

  parar();
  delay(50);
}


// ============================================
// 7. INICIALIZACAO
// ============================================

void setup() {

  // Configuracao dos motores
  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  pinMode(STBY, OUTPUT);

  // Sensores de borda
  pinMode(SENSOR_ESQUERDO, INPUT);
  pinMode(SENSOR_DIREITO, INPUT);

  // HC-SR04
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Botao com resistor interno
  pinMode(BOTAO, INPUT_PULLUP);

  // Monitor serial
  Serial.begin(9600);

  // Habilita a ponte H
  digitalWrite(STBY, HIGH);

  parar();

  Serial.println("ROBO SUMO");
  Serial.println("Aguardando acionamento...");

  // Aguarda o botao ser pressionado
  while (digitalRead(BOTAO) == HIGH) {
    parar();
  }

  // Aguarda o botao ser solto
  while (digitalRead(BOTAO) == LOW) {
    parar();
  }

  Serial.println("Preparando combate...");

  // Cinco segundos de espera
  delay(TEMPO_INICIO);

  Serial.println("COMBATE INICIADO!");
}


// ============================================
// 8. LOOP PRINCIPAL
// ============================================

void loop() {

  // PRIORIDADE 1: PROTEGER O ROBO

  bool esquerda = detectarBordaEsquerda();
  bool direita = detectarBordaDireita();

  if (esquerda || direita) {

    Serial.println("BORDA DETECTADA!");

    escaparBorda();

    return;
  }


  // PRIORIDADE 2: PROCURAR ADVERSARIO

  float distancia = medirDistancia();

  Serial.print("Distancia: ");
  Serial.println(distancia);


  // PRIORIDADE 3: ATACAR

  if (distancia > 0 &&
      distancia <= DISTANCIA_ATAQUE) {

    Serial.println("ADVERSARIO DETECTADO!");

    avancar(VELOCIDADE_ATAQUE);

  }

  // PRIORIDADE 4: CONTINUAR PROCURANDO

  else {

    Serial.println("PROCURANDO ADVERSARIO...");

    girarDireita(VELOCIDADE_BUSCA);

  }

  delay(30);
}
