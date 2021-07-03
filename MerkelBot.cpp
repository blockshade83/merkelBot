#include "MerkelBot.h"

MerkelBot::MerkelBot()
{
	
}

void MerkelBot::init() 
{
    // initialize CSV Reader object
    CSVReader csvReader{};

    // read order book from the file
    fullOrderBook = csvReader.readCSV("20200317.csv");

    // copy vector of timestamps to class variable
    for (auto const& e : fullOrderBook.timestamps)
    {
        allTimestamps.push_back(e);
        orders[e] = fullOrderBook.ordersByTimestamp[e];
    }

    // copy vectof of products to class variable
    for (auto const& e : fullOrderBook.products)
    {
        allProducts.push_back(e);
    }

    // Assign file to filestream object for logging bot assets
    botAssetsLog.open("BotAssetsLog.txt");

    // Assign file to filestream object for logging bot orders
    botActiveOrdersLog.open("BotActiveOrdersLog.csv");
    botActiveOrdersLog << "Timestamp,Product,Price,Amount,Corresponding_Amount,Order_Type,Username,Status" << std::endl;

    botCancelledOrdersLog.open("BotCancelledOrdersLog.csv");
    botCancelledOrdersLog << "Timestamp,Product,Price,Amount,Corresponding_Amount,Order_Type,Username,Status" << std::endl;

    // Assign file to filestream object for logging bot sales
    botSalesLog.open("BotSalesLog.csv");
    botSalesLog << "Timestamp,Product,Price,Order_Type,Amount,Currency_1,Corresponding_Amount,Currency_2,";
    botSalesLog << "Max_Bid_Price,Min_Ask_Price,Average_Price" << std::endl;

    // initialzing wallets and standard order amounts
    botAssets.addFunds();
    botAssets.setStandardOrderAmounts();

    // iterating through timestamp values until we reaches the end, so we are stopping at latestTimestamp
    for (int i = 0; i < allTimestamps.size() - 1; ++i)
    {   
        currentTime = allTimestamps[i];
        // run all bot operations related to current timestamp: cancel orders, place asks/bids
        processBotActions();

        // writing to assets log
        botAssetsLog << "Timestamp: " << currentTime << std::endl;
        botAssetsLog << "Before processing sales: " << std::endl;
        botAssetsLog << "========================= " << std::endl;
        botAssets.logAssets(botAssetsLog);

        // match asks to bids and log sales
        runMarketSales();

        // transferring active user orders to next period
        for (OrderBookEntry& order: fullOrderBook.activeUserOrders)
        {
            order.orderStatus = "carryover";
            orders[allTimestamps[i+1]].push_back(order);
        }

        // empty the list of active user orders
        fullOrderBook.activeUserOrders.clear();

        // writing to assets log
        botAssetsLog << "After processing sales: " << std::endl;
        botAssetsLog << "========================= " << std::endl;

        // writing to assets log
        botAssets.logAssets(botAssetsLog);
        logTotalAssetsUSD(botAssetsLog);
        logSalesImpact(botAssetsLog);     
    } 

    // process bot operations for last timestamp
    processBotActions();
    // writing to assets log
    botAssetsLog << "Timestamp: " << currentTime << std::endl;
    botAssets.logAssets(botAssetsLog);

    // close log files
    botActiveOrdersLog.close();
    botAssetsLog.close();
    botSalesLog.close();
}

/** function to execute all actions, including bot decisions for current time*/
void MerkelBot::processBotActions()
{
    // getting current market prices
    getMarketPrices();
    // adding most recent prices to historical prices
    avgHistoricalPrices.push_back(avgCurrentPrices);

    // only 1 data point available at the beginning and no price prediction is possible
    // checking if we moved beyond the initial timestamp
    if (currentTime != allTimestamps[0]) 
    {
        // updating prediction for estimated prices in the next period
        updatePricePrediction();

        // cancelling orders from previous periods if applicable
        cancelBotOrders();

        // placing bot asks and bids
        placeBotAsks();
        placeBotBids();
    }
    
    // iterating through all live orders to record carryover orders to the logs
    for (OrderBookEntry& order : orders[currentTime])
    {
        if (order.username == "botuser" && order.orderStatus != "cancelled")
        {
            logBotOrder(order, botActiveOrdersLog);
        }
    }
}

/** function to extract bids by product from the current orders*/
std::vector<OrderBookEntry> MerkelBot::getLiveBidsForProduct(std::string const product)
{
    // define empty vector to store relevant orders
    std::vector<OrderBookEntry> orders_sub;
    // iterate through the orders for current timestamp
    for (OrderBookEntry& e : orders[currentTime])
    {
        if (e.product == product && e.orderType == OrderBookType::bid)
        {
            orders_sub.push_back(e);
        }
    }
    return orders_sub;
}

/** function to extract asks by product from the current orders*/
std::vector<OrderBookEntry> MerkelBot::getLiveAsksForProduct(std::string const product)
{
    // define empty vector to store relevant orders
    std::vector<OrderBookEntry> orders_sub;
    // iterate through the orders for current timestamp
    for (OrderBookEntry& e : orders[currentTime])
    {
        if (e.product == product && e.orderType == OrderBookType::ask)
        {
            orders_sub.push_back(e);
        }
    }
    return orders_sub;
}

/** get current market prices */
void MerkelBot::getMarketPrices() 
{
    // empty vectors for bids and asks
    std::vector<OrderBookEntry> product_bid_orders;
    std::vector<OrderBookEntry> product_ask_orders;

    // iterating through list of known products
    for (std::string const& p : allProducts)
    {
        // extracting bids and asks for current product
        std::vector<OrderBookEntry> product_bid_orders = getLiveBidsForProduct(p);
        // std::cout << "Bids for product " << p << ": " << product_bid_orders.size() << std::endl; 
        std::vector<OrderBookEntry> product_ask_orders = getLiveAsksForProduct(p);
        
        // checking if we have at least an order
        // if no bid orders exist, current price will be assumed to be the last known price
        if (product_bid_orders.size() > 0)
        {
            // initializing max and min values with the first order
            maxBidPrices[p] = product_bid_orders[0].price;
            // minBidPrices[p] = product_bid_orders[0].price;

            // using a try/catch approach to avoid stopping the program
            try
            {
                // iterating through all the bid orders
                for (OrderBookEntry& e : product_bid_orders)
                {                    
                    // if we find a higher price, we update the max price
                    if (e.orderType == OrderBookType::bid && e.price > maxBidPrices[p])
                    {
                        maxBidPrices[p] = e.price;
                    }
                }
            }
            catch(const std::exception& e)
            {
                std::cout << e.what() << '\n';
                continue;
            }
        } // end if

        // checking if we have at least an order
        // if no ask orders exist, current price will be assumed to be the last known price
        if (product_ask_orders.size() > 0)
        {

            // maxAskPrices[p] = product_ask_orders[0].price;
            minAskPrices[p] = product_ask_orders[0].price;

            // using a try/catch approach to avoid stopping the program
            try
            {
                for (OrderBookEntry& e : product_ask_orders)
                {   
                    // if we find a lower price, we update the min price
                    if (e.orderType == OrderBookType::ask && e.price < minAskPrices[p])
                    {
                        minAskPrices[p] = e.price;
                    }
                }
            }
            catch(const std::exception& e)
            {
                std::cout << e.what() << '\n';
                continue;
            }
        } // end if

        // updating average price
        if (minAskPrices[p] < maxBidPrices[p])  // if there are no bids in the current period, the max bid may be from a previous timestamp
        {
            avgCurrentPrices[p] = minAskPrices[p];
        }
        else // we use the average price (middle of spread between max bid and min ask)
        {
            avgCurrentPrices[p] = (minAskPrices[p] + maxBidPrices[p]) / 2;
        }
    } // end for loop
}

/** executing and logging sales */
void MerkelBot::runMarketSales()
{
    // iterating through all known products
    for (std::string& p : allProducts)
    {
        // matching asks to bids and generating sales for current time
        std::vector<OrderBookEntry> sales = fullOrderBook.matchAsksToBids(orders[currentTime], p, currentTime);

        // iterating through the list of sales
        for (OrderBookEntry& sale: sales)
        {   
            // updating sale logs and wallets for the bot
            if (sale.username == "botuser")
            {
                logBotSale(sale, botSalesLog);
                botAssets.processSale(sale);
            }
        }
    }
}

/** function to estimate next likely value of a numerical array using linear regression*/
long double MerkelBot::linRegressionPrediction(std::vector<long double>& priceHistory)
{
    // storing size of the array
    int n = priceHistory.size();
    // temporary variables for the calculations
    long double sumX=0, sumY=0, upperTerm = 0, lowerTerm = 0;
    long double meanX, meanY, intercept, coeff;

    try
    {
        // taking at most the last 14 data points, assuming timesteps of equal distance
        for (int i = std::max(n - 14, 1); i < n; ++i)
        {
            sumX += i;
            sumY += priceHistory[i];
        }
        meanX = sumX / n;
        meanY = sumY / n;

        // intermediary calculations
        for (int i = 1; i <= n; ++i)
        {
            upperTerm += (i - meanX) * (priceHistory[i-1] - meanY);
            lowerTerm += (i - meanX) * (i - meanX);
        }

        // calculating linear regression coefficient and intercept
        coeff = upperTerm / lowerTerm;
        intercept = meanY - coeff * meanX;
    }
    catch(const std::exception& e)
    {
        std::cout << "MerkelBot::linRegressionPrediction invalid input" << std::endl;
        throw; // throw exception to the calling function
    }

    // returning prediction 5 intervals later
    return intercept + coeff * (n + 5);
}

/** function to extract historical prices from a specific product*/
std::vector<long double> MerkelBot::getHistoricalPricesByProduct(std::string const product)
{
    // vector to store the historical values for a product
    std::vector<long double> productPriceHistory;
    // iterating through the list of historical prices
    for (std::map<std::string, long double>& priceTimeSlice : avgHistoricalPrices)
    {
        productPriceHistory.push_back(priceTimeSlice[product]);
    }

    return productPriceHistory;
}

/** update price prediction based on the historical and the most recent market prices*/
void MerkelBot::updatePricePrediction()
{
   // vector to store historical prices for a product
   std::vector<long double> productPriceHistory;
   for (std::string const& p : allProducts)
   {
       try
       {
           productPriceHistory = getHistoricalPricesByProduct(p);
           pricePrediction[p] = linRegressionPrediction(productPriceHistory);
       }
       catch(const std::exception& e)
       {
           std::cout << e.what() << '\n';
       }
   } 
}

/** analyze current market and place bot bids */ 
void MerkelBot::placeBotBids()
{
    long double  bidPrice, buyAmount, sellAmount;
    for (std::string const& p : allProducts)
    {
        std::vector<std::string> currs = CSVReader::tokenise(p, '/');
        
        // initializing bid price above current bidding, at min ask
        bidPrice = minAskPrices[p];
        // initialing amount with the standard order amount
        buyAmount = botAssets.standardOrderAmount[currs[0]];
        // calculate sell amount
        sellAmount = buyAmount * bidPrice;

        // using stringstream to avoid losing decimals when converting to string, applicable to DOGE/BTC
        std::stringstream priceString;
        priceString << bidPrice;

        // check if the price prediction is higher than current market price
        // if we expect the market price to increase, we should buy at the lowest price we can get now
        if (pricePrediction[p] > avgCurrentPrices[p] && bidPrice > 0 && bidPrice < pricePrediction[p])
        {
            try
            {
                OrderBookEntry obe = CSVReader::stringsToOBE(priceString.str(), 
                                                             std::to_string(buyAmount), 
                                                             currentTime, 
                                                             p, 
                                                             OrderBookType::bid);
                obe.username = "botuser"; 

                // check if enough funds are available
                if (botAssets.standardWallet.canFulfillOrder(obe))
                {
                    // add order to order book
                    orders[currentTime].push_back(obe);

                    // move order amount for product to the reserved wallet to avoid placing uncovered bids/asks
                    botAssets.blockAmount(currs[1], sellAmount);
                }
            }
            catch(const std::exception& e)
            {
                std::cout << "MerkelBot::placeBotBids Bad input" << std::endl;
            }
        
        } // end if 
    } // end for    
}

/** analyze current market and place bot asks */ 
void MerkelBot::placeBotAsks()
{
    long double askPrice, buyAmount, sellAmount;
    for (std::string const& p : allProducts)
    {
        std::vector<std::string> currs = CSVReader::tokenise(p, '/');

        // no trading happening on this product
        if ( (minAskPrices[p] == 0) && (maxBidPrices[p] == 0) )
            continue;
        
        // initializing ask price below current ask, at max bid
        askPrice = avgCurrentPrices[p];
        // initialing amount with the standard order amount
        sellAmount = botAssets.standardOrderAmount[currs[0]];
        // calculate buy amount
        buyAmount = sellAmount * askPrice;

        // using stringstream to avoid losing decimals when converting to string, applicable to DOGE/BTC
        std::stringstream priceString;
        priceString << askPrice;

        // check if the price prediction is at leasr 0.1% lower than current market price
        // if we expect the market price to decrease, we should sell at the highest price we can get now
        if (pricePrediction[p] < 0.995 * avgCurrentPrices[p] && askPrice > pricePrediction[p])
        {
            try
            {
                OrderBookEntry obe = CSVReader::stringsToOBE(priceString.str(), 
                                                             std::to_string(sellAmount), 
                                                             currentTime, 
                                                             p, 
                                                             OrderBookType::ask);
                obe.username = "botuser";

                // check if enough funds are available
                if (botAssets.standardWallet.canFulfillOrder(obe))
                {
                    // add order to order book
                    orders[currentTime].push_back(obe);

                    // move order amount for product to the reserved wallet to avoid placing uncovered bids/asks
                    botAssets.blockAmount(currs[0], sellAmount);
                }
            }
            catch(const std::exception& e)
            {
                std::cout << "MerkelBot::placeBotBids Bad input" << std::endl;
            }
        
        } // end if 
    } // end for    
}

/** analyze current market and cancel carryover orders where needed */ 
void MerkelBot::cancelBotOrders()
{
    std::vector<std::string> currs;
    for (OrderBookEntry& order : orders[currentTime])
    {
        if (order.orderStatus == "carryover" && order.username == "botuser")
        {
            if(order.orderType == OrderBookType::ask)
            {
                // check order price vs. current market prices
                // if the price is lower than bidding prices or we predict the price will increase, we cancel the order
                if (order.price < maxBidPrices[order.product] | order.price < pricePrediction[order.product])
                {
                    order.orderStatus = "cancelled";
                    currs = CSVReader::tokenise(order.product, '/');
                    botAssets.unblockAmount(currs[0], order.amount);

                    logBotOrder(order, botCancelledOrdersLog);
                }
            }
            if(order.orderType == OrderBookType::bid)
            {
                // check order price vs. current market
                // if price is higher than asking prices or we predict the price will decrease, we cancel the order
                if (order.price > minAskPrices[order.product] | order.price > pricePrediction[order.product])
                {
                    order.orderStatus = "cancelled";
                    currs = CSVReader::tokenise(order.product, '/');
                    botAssets.unblockAmount(currs[1], order.amount * order.price);

                    logBotOrder(order, botCancelledOrdersLog);
                }
            }
        }
    }                             
}

/** add bot order to the order log */
void MerkelBot::logBotOrder(OrderBookEntry& order, std::ofstream& logFile)
{
    logFile << order.timestamp << ",";
    logFile << order.product << ",";
    logFile << order.price << ",";
    logFile << std::to_string(order.amount) << ",";
    logFile << std::to_string(order.price * order.amount) << ",";
    // add corresponding amounts and oroder type
    if (order.orderType == OrderBookType::bid)
    {
        logFile << "bid";
    }
    if (order.orderType == OrderBookType::ask)
    {
        logFile << "ask";
    }
    logFile << ",";
    logFile << order.username << ",";
    logFile << order.orderStatus << std::endl;
}

/** add sale to the sale log*/
void MerkelBot::logBotSale(OrderBookEntry& sale, std::ofstream& logFile)
{
    std::vector<std::string> currs = CSVReader::tokenise(sale.product, '/');

    // log sale
    logFile << currentTime << ",";
    logFile << sale.product << ",";
    logFile << sale.price << ",";
    // add corresponding amounts and sale type
    if (sale.orderType == OrderBookType::bidsale)
    {
        logFile << "bidsale,";
        logFile << std::to_string(sale.amount) << ",";
        logFile << currs[0] << ",";
        logFile << std::to_string(-sale.amount * sale.price);

        salesImpactOnAssets[currs[0]] += sale.amount;
        salesImpactOnAssets[currs[1]] += -sale.amount * sale.price;
    }

    if (sale.orderType == OrderBookType::asksale)
    {
        logFile << "asksale,";
        logFile << std::to_string(-sale.amount) << ",";
        logFile << currs[0] << ",";
        logFile << std::to_string(sale.amount * sale.price);

        salesImpactOnAssets[currs[0]] += -sale.amount;
        salesImpactOnAssets[currs[1]] += sale.amount * sale.price;        
    }

    logFile << "," << currs[1] << ",";
    logFile << maxBidPrices[sale.product] << ",";
    logFile << minAskPrices[sale.product] << ",";
    logFile << avgCurrentPrices[sale.product] << std::endl;
}

/** log total value of assets in USD equivalent */
void MerkelBot::logTotalAssetsUSD(std::ofstream& logFile)
{
    double totalAssetsUSD = 0;
    std::string product;

    // calculating USD equivalent for all the assets combined to evaluate the evolution of our portfolio
    for (std::pair<std::string, double> pair : botAssets.totalAssets.currencies)
    {
        if (pair.first == "USDT")
        {
            totalAssetsUSD += pair.second;
            logFile << pair.first << " " << pair.second << " * " << 1.00 << " = " << pair.second << std::endl;
        }
        else
        {
            product = pair.first + "/USDT";
            totalAssetsUSD += pair.second * avgCurrentPrices[product];
            logFile << pair.first << " " << std::to_string(pair.second) << " * ";
            logFile << std::to_string(avgCurrentPrices[product]) << " = USD ";
            logFile << std::to_string(pair.second * avgCurrentPrices[product]) << std::endl;
        }
        
    }
    logFile << "Total assets in USD equivalent as on " << currentTime;
    logFile << ": " << "USD " << totalAssetsUSD << std::endl;
    logFile << "===================================" << std::endl;
}

/** log impact of cumulative sales to assets log - assists with checks */
void MerkelBot::logSalesImpact(std::ofstream& logFile)
{
    logFile << "Impact of cumulative sales on assets as per sales log: " << std::endl;
    std::string s = "";
    for (std::pair<std::string, double> pair : salesImpactOnAssets)
    {
        std::string currency = pair.first;
        double amount = pair.second;
        s += currency + " : " + std::to_string(amount) + "\n";
    }
    logFile << s << std::endl;  
}