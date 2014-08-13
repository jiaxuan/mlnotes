

fx_day_roll_server checks FXTradingDateRoll periodically and whenever a change 
is detected, it updates FXTradingDate through db broker.

	class FXTradingDateRoll version 1.0
		//ccy code or ccy pair (use pairs to handle market conventions that have complex precedence relations)
		ccyLabel varchar(6);

		//day of week, 1 is Sunday
		dayOfWeek int32;
		
		//roll hour
		rollHour int32;
		
		//roll minute
		rollMinute int32;

		//roll offset, i.e. the trading day after the roll will be local date + offset days
		rollOffset int32;    
		
		//time zone of roll time - MUST be expressed in a form suitable for a call to putenv(TZ=...)
		timeZone varchar(60);



	// A table to store the current FX trading date.
	class FXTradingDate version 1.0 apiexposed
		//tradingDate
		tradingDate tradingdate;
		
		//ccy code or ccy pair (use pairs to handle market conventions that have complex precedence relations)
		ccyLabel varchar(6);	
		
