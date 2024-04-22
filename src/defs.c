
bool floats_equal(float a, float b) {
  return (a >= b - ath_epsilon && a <= b + ath_epsilon);
}