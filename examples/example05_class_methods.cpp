/*
 * C++ file for generated class methods.
 */

#include <iostream>
#include <string>

// Class for a Celsius temperature.
class Temperature
{
public:
    // Initialize a Celsius temperature.
    explicit Temperature(double celsius)
        : m_celsius(celsius)
    {
        // Empty.
    }

    // Convert the temperature to Fahrenheit.
    __llm__ double fahrenheit() const
    {
        Convert the stored Celsius temperature to Fahrenheit.
    }

    // Describe the temperature.
    __llm__ std::string describe() const
    {
        Return a concise human-friendly description of the stored Celsius
        temperature, including both Celsius and Fahrenheit values.
    }

private:
    // Stored temperature in degrees Celsius.
    double m_celsius;
};

// Run the class-method example.
int main()
{
    Temperature temperature(21.5);
    std::cout << temperature.fahrenheit() << '\n';
    std::cout << temperature.describe() << '\n';
}
