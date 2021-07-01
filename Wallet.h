#pragma once
#include <string>
#include <map>
#include "OrderBookEntry.h"

class Wallet
{
    public:
        Wallet();
        /** insert currency to the wallet */
        void insertCurrency(std::string type, double amount);
        
        /** remove currency from the wallet */
        bool removeCurrency(std::string type, double amount);
        
        /** check if the wallet contain this much currency or more */
        bool containsCurrency(std::string type, double amount);
        
        /** checks if the wallet can support with this ask or bid */
        bool canFulfillOrder(OrderBookEntry order);

        /** generate a string representation of the wallet*/
        std::string toString();

        /** add wallet object type to << operator */
        friend std::ostream& operator<<(std::ostream& os, Wallet& wallet);

        // moved to public so that we can access the wallet balance from other places
        std::map<std::string, double> currencies;
    
    private:
};
