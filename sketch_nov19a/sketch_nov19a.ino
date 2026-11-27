
/*
 * ==========================================
 *       ROBO DE SUMO - ARDUINO UNO
 * ==========================================
 *
 * Controlador: L298N
 * Sensores: 3x TCRT5000
 * Distancia: HC-SR04
 *
 * ESTRATEGIA:
 * 1. Preparacao inicial
 * 2. Busca pelo adversario
 * 3. Ataque
 * 4. Fuga da borda
 *
 */

// ==========================================
// PINAGEM
// ==========================================

// Sensores de borda
const byte SENSOR_ESQUERDO = 3;
const byte SENSOR_CENTRAL  = 4;
const byte SENSOR_DIREITO  = 5;

// Ultrassonico
const byte TRIG = 6;
const byte ECHO = 7;

// Ponte H L298N
const byte IN1 = 8;
const byte IN2 = 9;
const byte IN3 = 10;
const byte IN4 = 1;

// ==========================================
// CONFIGURACOES
// ==========================================

// ATENCAO: ajustar conforme a resposta real
// dos sensores na superficie da arena.
const byte BORDA_ATIVA = LOW;

// Inversao individual dos motores.
// Alterar caso algum motor avance ao contrario.
const bool INVERTER_MOTOR_A = false;
const bool INVERTER_MOTOR_B = false;

// Distancias de combate
const float DISTANCIA_ATAQUE = 35.0;
const float DISTANCIA_LIBERAR_ATAQUE = 45.0;

// Preparacao
const unsigned long TEMPO_PREPARACAO = 5000;

// Manobra de fuga
const unsigned long TEMPO_RECUO = 350;
const unsigned long TEMPO_GIRO_MINIMO = 350;
const unsigned long TEMPO_GIRO_MAXIMO = 1200;

// Estabilidade dos sensores apos a fuga
const unsigned long TEMPO_BORDA_LIVRE = 100;

// Busca
const unsigned long INTERVALO_ULTRASSOM = 60;

// ==========================================
// ESTADOS DO ROBO
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

// Controle de tempo
unsigned long inicioPreparacao = 0;
unsigned long inicioFuga = 0;
unsigned long inicioGiro = 0;
unsigned long inicioBordaLivre = 0;
unsigned long ultimaMedicao = 0;

// Controle de giro
int direcaoGiro = 1;

// ==========================================
// CONTROLE DOS MOTORES
// ==========================================

// Canal A: OUT1 e OUT2
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

// Canal B: OUT3 e OUT4
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
// SENSORES DE BORDA
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
    pulseIn(ECHO, HIGH, 25000UL);

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

  recuar();
}

// ==========================================
// EXECUCAO DA FUGA
// ==========================================

void executarFuga() {

  unsigned long agora = millis();

  if (etapaFuga == RECUANDO) {

    recuar();

    if (agora - inicioFuga >= TEMPO_RECUO) {

      etapaFuga = GIRANDO;

      inicioGiro = agora;
      inicioBordaLivre = 0;

      girar(direcaoGiro);
    }

    return;
  }

  // Durante o giro, continua verificando
  // se os sensores ainda identificam a borda.

  girar(direcaoGiro);

  bool borda = bordaDetectada();

  if (borda) {

    inicioBordaLivre = 0;

  } else {

    if (inicioBordaLivre == 0) {
      inicioBordaLivre = agora;
    }
  }

  bool giroMinimoConcluido =
    agora - inicioGiro >= TEMPO_GIRO_MINIMO;

  bool bordaLivreEstavel =
    inicioBordaLivre != 0 &&
    agora - inicioBordaLivre >= TEMPO_BORDA_LIVRE;

  bool tempoMaximoConcluido =
    agora - inicioGiro >= TEMPO_GIRO_MAXIMO;

  if (
    giroMinimoConcluido &&
    (bordaLivreEstavel || tempoMaximoConcluido)
  ) {

    pararMotores();

    // Alterna o sentido da proxima fuga
    direcaoGiro = -direcaoGiro;

    estado = BUSCA;
  }
}

// ==========================================
// INICIALIZACAO DO ARDUINO
// ==========================================

void setup() {

  // Sensores de borda
  pinMode(SENSOR_ESQUERDO, INPUT);
  pinMode(SENSOR_CENTRAL, INPUT);
  pinMode(SENSOR_DIREITO, INPUT);

  // Ultrassonico
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  digitalWrite(TRIG, LOW);

  // L298N
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

  // FUGA
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
    }

    return;
  }

  // LEITURA PERIODICA DO ULTRASSONICO

  if (
    millis() - ultimaMedicao >=
    INTERVALO_ULTRASSOM
  ) {

    ultimaMedicao = millis();

    float distancia = medirDistancia();

    // Verifica novamente a borda
    // depois da medicao ultrassonica.

    if (bordaDetectada()) {

      iniciarFuga();
      return;
    }

    // ATAQUE
    if (estado == BUSCA) {

      if (distancia <= DISTANCIA_ATAQUE) {

        estado = ATAQUE;
        avancar();

      } else {

        girar(1);
      }
    }

    // MANTEM O ATAQUE ATE A DISTANCIA DE LIBERACAO
    else if (estado == ATAQUE) {

      if (distancia > DISTANCIA_LIBERAR_ATAQUE) {

        estado = BUSCA;
        girar(1);

      } else {

        avancar();
      }
    }
  }
}

