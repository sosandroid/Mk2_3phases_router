// Générer des tests unitaires au moment de la compilation
// source: http://softwarephilosophy.ninja/compile-time-unit-testing
// Objectif: utiliser des fonction statiques, évaluées au moment de la compilation.
// Si une évaluation retourne "false" la compilation échoue
// Avantage : détecter une erreur de configuration
// Inconvénient: difficilement maintenable car toutes les combinaisons doivent ajoutées à la main

constexpr bool pin_differs(uint8_t a, uint8_t b) {
	if (a == 0xff || b == 0xff || a != b) {
		return true;
	}
	return false;
}
constexpr bool feature_has_pin(bool feature, uint8_t pin) {
	if(!feature) return true;
	if(feature && pin != 0xff) return true;
	return false;
}
#define STATIC_ASSERT_PIN_DIFFERS(pin1, pin2) static_assert(pin_differs(pin1, pin2), "Pin assignation error, can not work, please check pins setup and number " #pin2);
#define STATIC_ASSERT_FEATURE_HAS_PIN(feature, pin) static_assert(feature_has_pin(feature, pin), "Some features do not have pin assignement");

namespace pindistribution_tests {
	STATIC_ASSERT_FEATURE_HAS_PIN(DUAL_TARIFF, offPeakForcePin);
	STATIC_ASSERT_PIN_DIFFERS(offPeakForcePin, tempSensorPin);
	STATIC_ASSERT_PIN_DIFFERS(offPeakForcePin, watchDogPin);
	STATIC_ASSERT_PIN_DIFFERS(offPeakForcePin, diversionPin);
	STATIC_ASSERT_PIN_DIFFERS(offPeakForcePin, rotationPin);
	STATIC_ASSERT_PIN_DIFFERS(offPeakForcePin, forcePin);
	//Pas d'obligation de faire un contrôle de deux pins déjà testés
	STATIC_ASSERT_FEATURE_HAS_PIN(TEMP_SENSOR_PRESENT, tempSensorPin);
	STATIC_ASSERT_PIN_DIFFERS(tempSensorPin, watchDogPin);
	STATIC_ASSERT_PIN_DIFFERS(tempSensorPin, diversionPin);
	STATIC_ASSERT_PIN_DIFFERS(tempSensorPin, rotationPin);
	STATIC_ASSERT_PIN_DIFFERS(tempSensorPin, forcePin);
	//[...]
} //namespace pindistribution_tests
