double dq250_gear_ratio(uint8_t gear) {
  switch (gear) {
    case 1:
      return 3.462;
      break;
    case 2:
      return 2.050;
      break;
    case 3:
      return 1.300;
      break;
    case 4:
      return 0.902;
      break;
    case 5:
      return 0.914;
      break;
    case 6:
      return 0.756;
      break;
    default:
      return 1.0;
  }

/* alterative ratios
    switch (gear) {
    case 1:
      return 2.93;
      break;
    case 2:
      return 1.83;
      break;
    case 3:
      return 1.300;
      break;
    case 4:
      return 0.98;
      break;
    case 5:
      return 1.03;
      break;
    case 6:
      return 0.83;
      break;
    default:
      return 1.0;
  }
  */
}

double dq250_final(uint8_t gear) {
  return (gear == 5 || gear == 6) ? 3.043 : 4.118;
}

double dq250_speed(uint16_t rpm_in, uint8_t gear) {
  double tireCircumference = PI * 0.6;
  double rpm = (double)rpm_in * 1.0;
  double speed_mps = (rpm * tireCircumference) / (dq250_gear_ratio(gear) * dq250_final(gear) * 60);
  double vehicleSpeedTemp = speed_mps * 3.6;
  return vehicleSpeedTemp > 10 ? vehicleSpeedTemp : 1;  // ignore below 10km/h
}

void parseDSG() {
  if (vehicleRPM != 0 && gear != 0) {
    switch (lever) {
      case LEVER_D:
      case LEVER_S:
      case LEVER_TIPTRONIC_ON:
      case LEVER_TIPTRONIC_UP:
      case LEVER_TIPTRONIC_DOWN:
        dsgSpeed = dq250_speed(vehicleRPM, gear);
        break;
      case LEVER_P:
        dsgSpeed = 0;
      default:
        dsgSpeed = 0;
        break;
    }
  }
}