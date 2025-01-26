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
}

double dq250_final(uint8_t gear) {
	return (gear == 5 || gear == 6) ? 3.043 : 4.118;
}

double dq250_speed(uint16_t rpm_in, uint8_t gear) {
    double tireCircumference = PI * 0.6;
    double rpm = (double) rpm_in * 1.0;
    double speed_mps = (rpm * tireCircumference) /
    		(dq250_gear_ratio(gear) * dq250_final(gear) * 60);
    double vehicleSpeed = speed_mps * 3.6;
    return vehicleSpeed > 10 ? vehicleSpeed : 0; // ignore below 10km/h
}