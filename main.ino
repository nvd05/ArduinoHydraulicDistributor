#define HYDRAULIC_PISTON_MICROSECOND_MOVEMENT 0.00001 // (second: 1) / (milliseconds: 100) / (microseconds: 1000)
#define HYDRAULIC_PISTON_MAXIMUM_POSITION     4
#define HYDRAULIC_PISTON_MINIMUM_POSITION     0
#define HYDRAULIC_PISTON_INITIAL_POSITION     2

#define HYDRAULIC_SPOOL_MICROSECOND_MOVEMENT 0.00001 // (second: 1) / (milliseconds: 100) / (microseconds: 1000)
#define HYDRAULIC_SPOOL_MAXIMUM_POSITION     1
#define HYDRAULIC_SPOOL_MINIMUM_POSITION     -1
#define HYDRAULIC_SPOOL_INITIAL_POSITION     0

#define HYDRAULIC_ENGINE_BACKWARD_POSITION -1
#define HYDRAULIC_ENGINE_NEUTRAL_POSITION 0
#define HYDRAULIC_ENGINE_FORWARD_POSITION 1
#define HYDRAULIC_ENGINE_PIN_ONE 23
#define HYDRAULIC_ENGINE_PIN_TWO 25

/**
 * @brief Таймер
 */
class Timer
{
private:
	long _lastRunTime;

public:
	long get_elapsed_time()
	{
		long current_time = micros();
		long elapsed_time = current_time - _lastRunTime;

		_lastRunTime = current_time;
		return elapsed_time;
	}

	Timer() : _lastRunTime(micros())
	{
	}
};

/**
 * @brief Гидравлический помощник
 */
class HydraulicAssistant
{
private:
	double _microsecondMovement;
	double _expectedPosition;
	double _maximumPosition;
	double _minimumPosition;
	double _currentPosition;
	Timer _timer;

protected:
	/**
	 * @param backward направление назад
	 * @param neutral нейтральное направление
	 * @param forward направление вперед
	 */
	double get_direction(double backward, double neutral, double forward)
	{
		if (get_current_position() < _expectedPosition)
		{
			return forward;
		}

		if (get_current_position() > _expectedPosition)
		{
			return backward;
		}

		return neutral;
	}

	/**
	 * @param direction принимает числа от -1 до 1
	 */
	void move(double direction)
	{
		_currentPosition += direction * _microsecondMovement * _timer.get_elapsed_time();
		_currentPosition = constrain(get_current_position(), _minimumPosition, _maximumPosition);
	}

	/**
	 * @param movement движение за одну микросекунду
	 * @param maximum максимальное положение
	 * @param minimum минимальное положение
	 * @param initial начальное положение
	 */
	HydraulicAssistant(double movement, double maximum, double minimum, double initial) :
		_microsecondMovement(movement),
		_expectedPosition(initial),
		_maximumPosition(maximum),
		_minimumPosition(minimum),
		_currentPosition(initial)
	{
	}

public:
	void set_expected_position(double expected)
	{
		_expectedPosition = expected;
	}

	double get_expected_position()
	{
		return _expectedPosition;
	}

	double get_current_position()
	{
		return _currentPosition;
	}
};

/**
 * @brief Гидравлический двигатель
 */
class HydraulicEngine
{
private:
	double _currentDirection;

	int _pinTwo;
	int _pinOne;

public:
	void set_direction(int direction)
	{
		switch (direction)
		{
			case HYDRAULIC_ENGINE_BACKWARD_POSITION:
				digitalWrite(_pinTwo, HIGH);
				digitalWrite(_pinOne, LOW);
				break;

			case HYDRAULIC_ENGINE_NEUTRAL_POSITION:
				digitalWrite(_pinTwo, LOW);
				digitalWrite(_pinOne, LOW);
				break;

			case HYDRAULIC_ENGINE_FORWARD_POSITION:
				digitalWrite(_pinTwo, LOW);
				digitalWrite(_pinOne, HIGH);
				break;

			default:
				Serial.println("Incorrect direction");
				break;
		}

		_currentDirection = direction;
	}

	double get_direction()
	{
		return _currentDirection;
	}

	HydraulicEngine(int pin_one, int pin_two)
	{
		_pinOne = pin_one;
		_pinTwo = pin_two;
	}
};

/**
 * @brief Гидравлический золотник
 */
class HydraulicSpool : public HydraulicAssistant
{
private:
	HydraulicEngine _engine;

public:
	void update_direction()
	{
		double minimum = HYDRAULIC_ENGINE_BACKWARD_POSITION;
		double central = HYDRAULIC_ENGINE_NEUTRAL_POSITION;
		double maximum = HYDRAULIC_ENGINE_FORWARD_POSITION;

		double direction = get_direction(minimum, central, maximum);

		// Serial.println(String(minimum) + " " + String(central) + " " + String(maximum));
		// Serial.println(String(get_current_position()) + " " + String(get_expected_position()) + " " + String(direction));
		// Serial.println(direction);

		_engine.set_direction(direction);
	}

	void update_position()
	{
		move(_engine.get_direction());
	}

	HydraulicSpool(HydraulicEngine engine) : _engine(engine), HydraulicAssistant(
		HYDRAULIC_SPOOL_MICROSECOND_MOVEMENT,
		HYDRAULIC_SPOOL_MAXIMUM_POSITION,
		HYDRAULIC_SPOOL_MINIMUM_POSITION,
		HYDRAULIC_SPOOL_INITIAL_POSITION)
	{
	}
};

/**
 * @brief Гидравлический поршень
 */
class HydraulicPiston : public HydraulicAssistant
{
private:
	HydraulicSpool &_spool;

public:
	void update_direction()
	{
		double maximum = HYDRAULIC_SPOOL_MAXIMUM_POSITION;
		double minimum = HYDRAULIC_SPOOL_MINIMUM_POSITION;
		double central = HYDRAULIC_SPOOL_INITIAL_POSITION;

		double direction = get_direction(minimum, central, maximum);

		// Serial.println(String(minimum) + " " + String(central) + " " + String(maximum));
		// Serial.println(String(get_current_position()) + " " + String(get_expected_position()) + " " + String(direction));
		// Serial.println(direction);

		_spool.set_expected_position(direction);
	}

	void update_position()
	{
		move(_spool.get_current_position());
	}

	HydraulicPiston(HydraulicSpool &spool) : _spool(spool), HydraulicAssistant(
		HYDRAULIC_PISTON_MICROSECOND_MOVEMENT,
		HYDRAULIC_PISTON_MAXIMUM_POSITION,
		HYDRAULIC_PISTON_MINIMUM_POSITION,
		HYDRAULIC_PISTON_INITIAL_POSITION)
	{
	}
};

HydraulicEngine engine(HYDRAULIC_ENGINE_PIN_ONE, HYDRAULIC_ENGINE_PIN_TWO);
HydraulicSpool spool(engine);
HydraulicPiston piston(spool);

void setup()
{
	// Serial.begin(9600);
	// Serial.println("run");

	piston.set_expected_position(4);
	// spool.set_expected_position(0);
}

void loop()
{
	spool.update_position();
	piston.update_position();

	piston.update_direction();
	spool.update_direction();

	// String piston_current = String(piston.get_current_position());
	// String spool_current  = String(spool.get_current_position());

	// String piston_expected = String(piston.get_expected_position());
	// String spool_expected = String(spool.get_expected_position());

	// Serial.println("current: " + piston_current + " " + spool_current + ", expected: " + piston_expected + " " + spool_expected);
	// Serial.println(spool.mem);
}
