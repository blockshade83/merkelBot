#pragma once
#include <string>
#include "Wallet.h"
#include "CSVReader.h"
#include "OrderBookEntry.h"

class Assets
{
    public:
        Assets();
        Wallet standardWallet;
        Wallet reservedWallet;
        Wallet totalAssets;

        // map variable to store the standard order amounts for each product
        std::map<std::string, double> standardOrderAmount;

        /** add amounts to wallet */
        void addFunds();

        /** set standard order amount */
        void setStandardOrderAmounts();

        /** update the contents of the wallets after processing a sale */
        void processSale(OrderBookEntry& sale);

        /** move amounts from standard wallet to the reserved wallet */
        void blockAmount(std::string currency, double amount);

        /** move amounts from reserved wallet back to the standard wallet */
        void unblockAmount(std::string currency, double amount);

        /** update total assets: standard wallet + reserved wallet */
        void updateTotalAssets(std::string currency);

        /** update the contents of the wallets following order cancellation */
        void processOrderCancellation(OrderBookEntry& order, Wallet& stdWallet, Wallet& resWallet);

        /** function to log assets in both wallets (standard, reserved) */
        void logAssets(std::ofstream& logFile);

    private:

};