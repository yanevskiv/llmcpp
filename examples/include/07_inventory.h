/*
 * C++ header for inventory reservations.
 */
#ifndef EXAMPLES_EXAMPLE07_INVENTORY_H
#define EXAMPLES_EXAMPLE07_INVENTORY_H
/** Class for reserving units from a fixed stock. */
class Inventory
{
public:
    /**
     * Initialize inventory with unreserved stock.
     * @param stock Initial number of available units.
     */
    explicit Inventory(unsigned stock);
    /**
     * Reserve units when enough stock is available.
     * @param count Number of units to reserve.
     * @return Whether the reservation succeeded.
     */
    bool reserve(unsigned count);
    /**
     * Get the remaining available stock.
     * @return Number of unreserved units.
     */
    unsigned available() const;
    /**
     * Get the reserved stock.
     * @return Number of reserved units.
     */
    unsigned reserved() const;

private:
    /** Number of unreserved units. */
    unsigned m_available;
    /** Number of reserved units. */
    unsigned m_reserved;
};

#endif
