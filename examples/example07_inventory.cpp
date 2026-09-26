/*
 * C++ file for generated inventory reservations.
 */

// Example interface for reserving inventory.
#include "include/example07_inventory.h"

// Standard header for output.
#include <iostream>

// Initialize available and reserved stock.
Inventory::Inventory(unsigned stock)
    : m_available(stock)
    , m_reserved(0)
{
    // Empty.
}

// Reserve stock only when the full request can be satisfied.
__llm__ bool Inventory::reserve(unsigned count)
{
    If enough stock is available, move count units from available to reserved
    and return true. Otherwise leave the inventory unchanged and return false.
}

// Return the number of unreserved units.
unsigned Inventory::available() const
{
    return m_available;
}

// Return the number of reserved units.
unsigned Inventory::reserved() const
{
    return m_reserved;
}

// Reserve stock and print the resulting inventory.
int main()
{
    Inventory inventory(10);
    inventory.reserve(4);
    std::cout << "available=" << inventory.available() << " reserved=" << inventory.reserved()
              << '\n';
}
