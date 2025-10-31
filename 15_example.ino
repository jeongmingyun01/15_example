#include <Servo.h>

// ====== 핀/펄스 설정 ======
#define PIN_SERVO   10

// 서보 펄스 폭(μs) 범위: 장비에 맞게 보정
#define _DUTY_MIN   500     // 0°
#define _DUTY_NEU   1475    // 90°
#define _DUTY_MAX   3000    // 180° (필요 시 2500 등으로 조정)

// 업데이트 주기
#define INTERVAL_MS 20

// ====== 과제 단계 정의 ======
// 1단계: 60초 동안 180° 이동 (3°/s)
// 2단계: 300초 동안 90° 이동 (0.3°/s)
struct Phase {
  float start_deg;
  float span_deg;
  float duration_sec;
};
Phase phases[2] = {
  { 0.0, 180.0, 60.0 },   // Phase 1
  { 0.0,  90.0, 300.0 }   // Phase 2
};

Servo myservo;

// 상태 변수
unsigned long last_update_ms = 0;
int phase_idx = 0;
bool running = true;

float angle_curr = 0.0f;    // 현재 각도(도)
float angle_start = 0.0f;   // 단계 시작 각도
float angle_end   = 0.0f;   // 단계 목표 각도
float deg_per_interval = 0; // INTERVAL마다 이동할 각도(도)

// 각도 → 펄스 변환
inline float angleToDuty(float deg) {
  // 선형 보간: 0° => _DUTY_MIN, 180° => _DUTY_MAX
  deg = constrain(deg, 0.0f, 180.0f);
  return _DUTY_MIN + ( (_DUTY_MAX - _DUTY_MIN) * (deg / 180.0f) );
}

// 단계 초기화
void beginPhase(int idx) {
  Phase p = phases[idx];

  angle_start = p.start_deg;
  angle_end   = p.start_deg + p.span_deg;
  angle_end   = constrain(angle_end, 0.0f, 180.0f);

  // 등속 속도 계산: span / duration = deg/sec
  float deg_per_sec = p.span_deg / p.duration_sec;  // float 유지!
  deg_per_interval  = deg_per_sec * (INTERVAL_MS / 1000.0f);

  angle_curr = angle_start;
  myservo.writeMicroseconds( angleToDuty(angle_curr) );

  Serial.println();
  Serial.print("=== PHASE "); Serial.print(idx+1); Serial.println(" START ===");
  Serial.print("start_deg="); Serial.print(angle_start);
  Serial.print(", end_deg="); Serial.print(angle_end);
  Serial.print(", duration="); Serial.print(p.duration_sec); Serial.println("s");
  Serial.print("deg_per_interval="); Serial.println(deg_per_interval, 6);
}

void setup() {
  Serial.begin(57600);

  // (선택) attach에 안전 펄스 범위 지정
  myservo.attach(PIN_SERVO/*, _DUTY_MIN, _DUTY_MAX*/);

  // Phase 1부터 시작
  beginPhase(phase_idx);
  last_update_ms = millis();
}

void loop() {
  if (!running) return;

  unsigned long now = millis();
  if (now - last_update_ms < INTERVAL_MS) return;
  last_update_ms = now;

  // 현재 단계 진행
  float prev = angle_curr;
  if (angle_end >= angle_start) {            // 증가 이동
    angle_curr += deg_per_interval;
    if (angle_curr > angle_end) angle_curr = angle_end;
  } else {                                   // 감소 이동(이번 과제엔 없음)
    angle_curr -= deg_per_interval;
    if (angle_curr < angle_end) angle_curr = angle_end;
  }

  // 서보 출력
  myservo.writeMicroseconds( angleToDuty(angle_curr) );

  // 로깅 (필요 시 주석 처리)
  Serial.print("phase="); Serial.print(phase_idx+1);
  Serial.print(", angle_curr="); Serial.print(angle_curr, 3);
  Serial.print(", duty="); Serial.println( angleToDuty(angle_curr), 1 );

  // 목표 도달 판단
  bool reached = (prev == angle_curr); // 더 이상 변화 없음 = 끝
  if (reached) {
    // 다음 단계로
    phase_idx++;
    if (phase_idx >= 2) {
      Serial.println("=== ALL PHASES DONE (총 6분 실험 완료) ===");
      running = false;
      return;
    }
    beginPhase(phase_idx);
  }
}
