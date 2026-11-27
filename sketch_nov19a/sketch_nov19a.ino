
/*
 * ==========================================
 *          ROBO DE SUMO - ARDUINO UNO
 * ==========================================
 *
 * VERSAO A - DETECCAO DA BORDA EM LOW
 *
 * Tatame: preto
 * Borda: branca
 *
 * Controlador: L298N
 * Sensores: 3x TCRT5000
 * Distancia: HC-SR04
 *
 */

// ==========================================
// PINAGEM
// ==========================================

const byte SENSOR_ESQUERDO = 3;
const byte SENSOR_CENTRAL  = 4;
const byte SENSOR_DIREITO  = 5;

const byte TRIG = 6;
const byte ECHO = 7;

const byte IN1 = 8;
const byte IN2 = 9;
const byte IN3 = 10;
const byte IN4 = 11;

// ==========================================
// CONFIGURACOES
// ==========================================

// Branco = LOW
const byte BORDA_ATIVA = LOW;

const bool INVERTER_MOTOR_A = false;
const bool INVERTER_MOTOR_B = false;

const float DISTANCIA_ATAQUE = 35.0;
const float DISTANCIA_LIBERAR_ATAQUE = 45.0;

const unsigned long TEMPO_PREPARACAO = 5000;

const unsigned long TEMPO_RECUO = 350;
const unsigned long TEMPO_GIRO_MINIMO = 350;
const unsigned long TEMPO_GIRO_MAXIMO = 1200;

const unsigned long TEMPO_BORDA_LIVRE = 100;

const unsigned long INTERVALO_ULTRASSOM = 60;
const unsigned long TIMEOUT_ULTRASSOM = 25000UL;

// ==========================================
// ESTADOS
// ==========================================

enum Estado {
  PREPARACAO,
  BUSCA,
  ATAQUE,
  FUGA
};

enum EtapaFuga {
  RECUANDO,
  GIRANDO
};

Estado estado = PREPARACAO;
EtapaFuga etapaFuga = RECUANDO;

// ==========================================
// CONTROLE DE TEMPO
// ==========================================

unsigned long inicioPreparacao = 0;
unsigned long inicioFuga = 0;
unsigned long inicioGiro = 0;
unsigned long inicioBordaLivre = 0;
unsigned long ultimaMedicao = 0;

bool contandoBordaLivre = false;

int direcaoGiro = 1;

// ==========================================
// CONTROLE DOS MOTORES
// ==========================================

void motorA(int direcao) {

  if (INVERTER_MOTOR_A) {
    direcao = -direcao;
  }

  if (direcao > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  }
  else if (direcao < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  }
  else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }
}

void motorB(int direcao) {

  if (INVERTER_MOTOR_B) {
    direcao = -direcao;
  }

  if (direcao > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  }
  else if (direcao < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }
  else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }
}

// ==========================================
// MOVIMENTACAO
// ==========================================

void pararMotores() {
  motorA(0);
  motorB(0);
}

void avancar() {
  motorA(1);
  motorB(1);
}

void recuar() {
  motorA(-1);
  motorB(-1);
}

void girar(int direcao) {
  motorA(direcao);
  motorB(-direcao);
}

// ==========================================
// DETECCAO DA BORDA
// ==========================================

bool bordaDetectada() {

  bool esquerdo =
    digitalRead(SENSOR_ESQUERDO) == BORDA_ATIVA;

  bool central =
    digitalRead(SENSOR_CENTRAL) == BORDA_ATIVA;

  bool direito =
    digitalRead(SENSOR_DIREITO) == BORDA_ATIVA;

  return esquerdo || central || direito;
}

// ==========================================
// SENSOR ULTRASSONICO
// ==========================================

float medirDistancia() {

  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG, LOW);

  unsigned long duracao =
    pulseIn(ECHO, HIGH, TIMEOUT_ULTRASSOM);

  if (duracao == 0) {
    return 999.0;
  }

  return duracao / 58.0;
}

// ==========================================
// INICIALIZACAO DA FUGA
// ==========================================

void iniciarFuga() {

  estado = FUGA;
  etapaFuga = RECUANDO;

  inicioFuga = millis();

  inicioBordaLivre = 0;
  contandoBordaLivre = false;

  recuar();
}

// ==========================================
// EXECUCAO DA FUGA
// ==========================================

void executarFuga() {

  unsigned long agora = millis();

  // ETAPA 1: RECUAR

  if (etapaFuga == RECUANDO) {

    recuar();

    if (agora - inicioFuga >= TEMPO_RECUO) {

      etapaFuga = GIRANDO;

      inicioGiro = agora;
      inicioBordaLivre = 0;
      contandoBordaLivre = false;

      girar(direcaoGiro);
    }

    return;
  }

  // ETAPA 2: GIRAR

  bool borda = bordaDetectada();

  if (borda) {

    contandoBordaLivre = false;

  } else {

    if (!contandoBordaLivre) {

      inicioBordaLivre = agora;
      contandoBordaLivre = true;
    }
  }

  girar(direcaoGiro);

  bool giroMinimoConcluido =
    agora - inicioGiro >= TEMPO_GIRO_MINIMO;

  bool bordaLivreEstavel =
    contandoBordaLivre &&
    agora - inicioBordaLivre >= TEMPO_BORDA_LIVRE;

  bool giroMaximoConcluido =
    agora - inicioGiro >= TEMPO_GIRO_MAXIMO;

  // AREA SEGURA ENCONTRADA

  if (
    giroMinimoConcluido &&
    bordaLivreEstavel
  ) {

    pararMotores();

    direcaoGiro = -direcaoGiro;

    estado = BUSCA;

    ultimaMedicao = millis();

    return;
  }

  // BORDA AINDA DETECTADA

  if (giroMaximoConcluido && borda) {

    pararMotores();

    direcaoGiro = -direcaoGiro;

    etapaFuga = RECUANDO;

    inicioFuga = millis();

    contandoBordaLivre = false;

    recuar();

    return;
  }
}

// ==========================================
// INICIALIZACAO DO ARDUINO
// ==========================================

void setup() {

  pinMode(SENSOR_ESQUERDO, INPUT);
  pinMode(SENSOR_CENTRAL, INPUT);
  pinMode(SENSOR_DIREITO, INPUT);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  digitalWrite(TRIG, LOW);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pararMotores();

  inicioPreparacao = millis();
}

// ==========================================
// LOOP PRINCIPAL
// ==========================================

void loop() {

  // PRIORIDADE MAXIMA: BORDA

  if (estado != FUGA && bordaDetectada()) {
    iniciarFuga();
  }

  // EXECUCAO DA FUGA

  if (estado == FUGA) {
    executarFuga();
    return;
  }

  // PREPARACAO

  if (estado == PREPARACAO) {

    pararMotores();

    if (
      millis() - inicioPreparacao >=
      TEMPO_PREPARACAO
    ) {

      estado = BUSCA;
      ultimaMedicao = millis();
    }

    return;
  }

  // LEITURA DO ULTRASSONICO

  if (
    millis() - ultimaMedicao >=
    INTERVALO_ULTRASSOM
  ) {

    ultimaMedicao = millis();

    float distancia = medirDistancia();

    // VERIFICA NOVAMENTE A BORDA

    if (bordaDetectada()) {
      iniciarFuga();
      return;
    }

    // BUSCA

    if (estado == BUSCA) {

      if (distancia <= DISTANCIA_ATAQUE) {

        estado = ATAQUE;
        avancar();

      } else {

        girar(1);
      }
    }

    // ATAQUE

    else if (estado == ATAQUE) {

      if (
        distancia > DISTANCIA_LIBERAR_ATAQUE
      ) {

        estado = BUSCA;
        girar(1);

      } else {

        avancar();
      }
    }
  }
}
