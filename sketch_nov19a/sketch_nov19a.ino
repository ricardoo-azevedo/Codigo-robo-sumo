
/*
 * ==========================================
 *          ROBO DE SUMO - ARDUINO UNO
 * ==========================================
 *
 * VERSAO 3 - BASEADA NO PRIMEIRO CODIGO
 *
 * Controlador: L298N
 * Sensores de borda: 3x TCRT5000
 * Distancia: HC-SR04
 *
 * Tatame: preto
 * Borda: branca
 *
 * ESTRATEGIA:
 * 1. Preparacao inicial
 * 2. Busca pelo adversario
 * 3. Ataque
 * 4. Fuga com prioridade para os sensores
 *
 */

// ==========================================
// PINAGEM ORIGINAL
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

// CONFIGURACAO PROVISORIA:
// Branco = LOW
//
// Se a saida DO dos sensores identificar
// branco como HIGH, alterar para HIGH.
const byte BORDA_ATIVA = LOW;

// Inversao individual dos motores
const bool INVERTER_MOTOR_A = false;
const bool INVERTER_MOTOR_B = false;

// Distancias de combate
const float DISTANCIA_ATAQUE = 35.0;
const float DISTANCIA_LIBERAR_ATAQUE = 45.0;

// Preparacao
const unsigned long TEMPO_PREPARACAO = 5000;

// Fuga
const unsigned long TEMPO_RECUO = 350;
const unsigned long TEMPO_GIRO_MINIMO = 350;
const unsigned long TEMPO_GIRO_MAXIMO = 1200;

// Estabilidade da deteccao
const unsigned long TEMPO_BORDA_LIVRE = 100;

// Ultrassonico
const unsigned long INTERVALO_ULTRASSOM = 60;
const unsigned long TIMEOUT_ULTRASSOM = 25000UL;

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

// ==========================================
// CONTROLE DE TEMPO
// ==========================================

unsigned long inicioPreparacao = 0;
unsigned long inicioFuga = 0;
unsigned long inicioGiro = 0;
unsigned long inicioBordaLivre = 0;
unsigned long ultimaMedicao = 0;

// Indica se o temporizador de area segura iniciou
bool contandoBordaLivre = false;

// ==========================================
// CONTROLE DE GIRO
// ==========================================

// 1 = primeiro sentido
// -1 = sentido contrario
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

  } else if (direcao < 0) {

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);

  } else {

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

  } else if (direcao < 0) {

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);

  } else {

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

  // ----------------------------------------
  // ETAPA 1: RECUAR
  // ----------------------------------------

  if (etapaFuga == RECUANDO) {

    // Mantem os dois motores recuando.
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

  // ----------------------------------------
  // ETAPA 2: GIRAR
  // ----------------------------------------

  // Continua verificando os sensores.
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

  // ----------------------------------------
  // AREA SEGURA ENCONTRADA
  // ----------------------------------------

  if (
    giroMinimoConcluido &&
    bordaLivreEstavel
  ) {

    pararMotores();

    // Alterna o sentido da proxima fuga.
    direcaoGiro = -direcaoGiro;

    estado = BUSCA;

    ultimaMedicao = millis();

    return;
  }

  // ----------------------------------------
  // BORDA AINDA DETECTADA
  // ----------------------------------------

  if (giroMaximoConcluido && borda) {

    // Nao abandona a fuga.
    // Recua novamente e tenta sair
    // girando para o sentido contrario.

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

  // ----------------------------------------
  // PRIORIDADE MAXIMA: BORDA
  // ----------------------------------------

  if (estado != FUGA && bordaDetectada()) {

    iniciarFuga();
  }

  // ----------------------------------------
  // EXECUCAO DA FUGA
  // ----------------------------------------

  if (estado == FUGA) {

    executarFuga();

    return;
  }

  // ----------------------------------------
  // PREPARACAO
  // ----------------------------------------

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

  // ----------------------------------------
  // LEITURA DO ULTRASSONICO
  // ----------------------------------------

  if (
    millis() - ultimaMedicao >=
    INTERVALO_ULTRASSOM
  ) {

    ultimaMedicao = millis();

    float distancia = medirDistancia();

    // Verifica novamente a borda apos a leitura.
    if (bordaDetectada()) {

      iniciarFuga();

      return;
    }

    // --------------------------------------
    // BUSCA
    // --------------------------------------

    if (estado == BUSCA) {

      if (distancia <= DISTANCIA_ATAQUE) {

        estado = ATAQUE;

        avancar();

      } else {

        girar(1);
      }
    }

    // --------------------------------------
    // ATAQUE
    // --------------------------------------

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
