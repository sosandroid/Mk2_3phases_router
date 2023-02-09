// Générer des tests unitaires au moment de la compilation
// source: http://softwarephilosophy.ninja/compile-time-unit-testing
// Objectif: utiliser des fonction statiques, évaluées au moment de la compilation.
// Si une évaluation retourne "false" la compilation échoue
// Avantage : détecter une erreur de configuration
// Inconvénient: difficilement maintenable car toutes les combinaisons doivent ajoutées à la main

constexpr bool uint8_differs(uint8_t a, uint8_t b) {
	if (a == 0xff || b == 0xff || a != b) {
		return true;
	}
	return false;
}
#define STATIC_ASSERT_UINT8T_DIFFERS(pin1, pin2) static_assert(uint8_differs(pin1, pin2), #pin1" == " #pin2);

namespace pindistribution_tests {
	STATIC_ASSERT_UINT8T_DIFFERS(offPeakForcePin, tempSensorPin);
	STATIC_ASSERT_UINT8T_DIFFERS(offPeakForcePin, watchDogPin);
	STATIC_ASSERT_UINT8T_DIFFERS(offPeakForcePin, diversionPin);
	STATIC_ASSERT_UINT8T_DIFFERS(offPeakForcePin, rotationPin);
	STATIC_ASSERT_UINT8T_DIFFERS(offPeakForcePin, forcePin);
	STATIC_ASSERT_UINT8T_DIFFERS(tempSensorPin, watchDogPin);
	STATIC_ASSERT_UINT8T_DIFFERS(tempSensorPin, diversionPin);
	STATIC_ASSERT_UINT8T_DIFFERS(tempSensorPin, rotationPin);
	STATIC_ASSERT_UINT8T_DIFFERS(tempSensorPin, forcePin);
	//[...]
} //namespace pindistribution_tests