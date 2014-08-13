
# The `Currency` table:

	class Currency version 1.0 apiexposed
		// state enum
		enum RoundMode (ROUND_UP, TRUNCATE);
		
		// currency Type enum
		enum CurrencyType (
			NORMAL,
			NDF,
			ROE,
			LVN_SELL,
			LVN_BUY_SELL,
			OST,
			METAL_LONDON,
			METAL_ZURICH
		);

		primary key (isoCode)


	id     isoCode rank isLCT qtyPrecision roundMode ccySet   mapCcy ccySalesGroup type longName                    
	------ ------- ---- ----- ------------ --------- -------- ------ ------------- ---- --------------------------- 
	151001 AED     8210 N     2            0         European (null) EMEA          0    UAE Dirham                  
	433118 AL2     1890 N     2            0         Default  ALL    ROE/LVN       2    Albanian Lek                
	433087 AM2     1730 N     2            0         Default  AMD    ROE/LVN       2    Armenia Dram                
	433028 AN2     8280 N     2            0         Default  ANG    ROE/LVN       2    Netherland Antilles Guilder 
	433116 AO2     502  N     2            0         Default  AOA    ROE/LVN       2    Angola Kwanza               
	433034 AR2     8263 N     2            0         Default  ARS    ROE/LVN       2    Argentina Peso              
	39001  ARS     8266 N     2            0         Default  (null) NDF           0    Argentina Peso              
	297009 AR_     8264 N     2            0         LatAm    ARS    NDF           1    Argentina Peso              
	30     AUD     8800 N     2            0         Default  (null) G7            0    Australian Dollar           
	433029 AW2     8278 N     2            0         Default  AWG    ROE/LVN       2    Aruba Guilder               


# Rounding

	bool FXCurrencyUtils::roundCurrencyValue(Decimal &value, const std::string &ccy)
		FXCacheUtils::getCurrencyByCode(ccy, currency)
		return roundCurrencyValue(value, currency);


	bool FXCurrencyUtils::roundCurrencyValue(Decimal &value, const Currency::Ptr &currency)
		if( currency->getRoundMode() == Currency::ROUND_UP )
			value.round(currency->getQtyPrecision(), Decimal::ROUND_HALF_AWAY_FROM_ZERO);
		else 
			value.round(currency->getQtyPrecision(), Decimal::ROUND_TO_ZERO);

	typedef DecimalDecNumber Decimal;
	class DecimalDecNumber
	enum RoundingMode
	{
		ROUND_HALF_TO_POSITIVE_INFINITY, // Always round half up                          floor(this + 0.5)
		ROUND_HALF_TO_NEGATIVE_INFINITY, // Always round half down                        floor(this + 0.49999...)
		ROUND_TO_POSITIVE_INFINITY,      // Always round up                               ceil(this)
		ROUND_TO_NEGATIVE_INFINITY,      // Always round down                             floor(this)
		ROUND_AWAY_FROM_ZERO,            // Always round to maximise the absolute value   ceil(abs(this)) * sign(this)
		ROUND_TO_ZERO,                   // Always round to minimise the absolute value   floor(abs(this)) * sign(this)
		ROUND_HALF_AWAY_FROM_ZERO,       // Round half to maximise the absolute value     floor(abs(this) + 0.5) * sign(this)
		ROUND_HALF_TO_ZERO,              // Round half to minimise the absolute value     floor(abs(this) + 0.49999...) * sign(this)
		ROUND_HALF_TO_EVEN               // Statistical rounding                          round(this)
	};


# Currency Pair

	int32 FXCurrencyUtils::toCurrencyPair(const std::string & ccy1, const std::string & ccy2, std::string & ccyPair)
		int32 rank1, rank2;
		FXCacheUtils::getCurrencyRank(ccy1, rank1)
		FXCacheUtils::getCurrencyRank(ccy2, rank2)
		if( rank1 > rank2 )
			ccyPair = ccy1 + ccy2;
		else
			ccyPair = ccy2 + ccy1;

	int32 FXCurrencyUtils::getOtherCcy(const std::string & ccypair, const std::string & knownCcy, std::string & foundCcy)
		int32 ret = getOtherCcySilent(ccypair, knownCcy, foundCcy);

	int32 FXCurrencyUtils::getOtherCcySilent(const std::string & ccypair, const std::string & knownCcy, std::string & foundCcy)
		if (knownCcy == ccypair.substr(3, 3))
			foundCcy = ccypair.substr(0, 3);
		else if (knownCcy == ccypair.substr(0, 3))
			foundCcy = ccypair.substr(3, 3);

# Minimum Value Unit

	Decimal FXCurrencyUtils::getMinValueUnit(std::string const &ccy)
		FXCacheUtils::getCurrencyByCode(ccy, currency);
		return 1 / (10 ^ currency->getQtyPrecision())


# Get Cross Currency

These tables may be used:

- FXCcyPairCrosses

	id ccyPair startTime    endTime crossCcy 
	-- ------- ------------ ------- -------- 
	1  AUDJPY  SU 00:00 NYK (null)  USD      
	2  AUDUSD  SU 00:00 NYK (null)  (null)   
	3  CHFJPY  SU 00:00 NYK (null)  USD      
	4  EURCHF  SU 00:00 NYK (null)  (null)   
	24 USDCZK  SU 00:00 NYK (null)  EUR      
	25 USDDKK  SU 00:00 NYK (null)  EUR      

	select distinct(crossCcy) from FXCcyPairCrosses

	crossCcy 
	-------- 
	USD      
	(null)   
	EUR      

- FXCcyPairFwdCrosses

	id ccyPair startTime    endTime crossCcy 
	-- ------- ------------ ------- -------- 
	2  AUDUSD  SU 00:00 NYK         (null)   
	13 EURUSD  SU 00:00 NYK         (null)   
	14 GBPUSD  SU 00:00 NYK         (null)   
	15 NZDUSD  SU 00:00 NYK         (null)   
	19 USDCAD  SU 00:00 NYK         (null)   


	template < class CrossTable >
	ml::common::util::Nullable< std::string > getCrossCurrency(const ml::spec::fx::common::CurrencyPair & ccyPair)
	{
		Cache < CrossTable > &crossCache = Cache < CrossTable >::instance();
	
		typename CrossTable::PtrList ptrs;
		crossCache.select(CrossTable::Index::ccyPair_index(ccyPair.getName()), ptrs );
	
		// If we don't have it set as either direct, or an explicit cross, then
		// assume it's crossed through USD
		if (ptrs.empty())
			// We should be getting data back from the provider for anything that's not crossed through USD
			//  - ie, all that are not crossed, and all that are crossed through something other than usd
			// So if we don't have an entry, then we must assume that it's crossed through USD
			const std::string USD = "USD";
			return USD;
	
		if (ptrs.size() > 1)
			LWARN("Currency pair %s crossing through more than one currency", get_temp_cstr(ccyPair));
			return Nullable < std::string >();                         
	
		// Return the crossing currency we found
		typename CrossTable::Ptr ptr = ptrs[0];
	
		if(!is_null(ptr->getCrossCcy()))
			return ptr->getCrossCcy()->str();
			
		return Nullable < std::string >();                         

	mlc::util::Nullable<std::string> FXCurrencyUtils::getSpotCrossCurrency(ml::spec::fx::common::CurrencyPair const &ccyPair)
		return getCrossCurrency < FXCcyPairCrosses >(ccyPair);

	mlc::util::Nullable<std::string> FXCurrencyUtils::getSpotCrossCurrency(std::string const &ccyPair)
		FXCacheUtils::getCurrencyPair(ccyPair, ccyPairObject);
		return getSpotCrossCurrency(get_value(ccyPairObject));

	mlc::util::Nullable<std::string> FXCurrencyUtils::getFwdCrossCurrency(ml::spec::fx::common::CurrencyPair const &ccyPair)
		return getCrossCurrency<FXCcyPairFwdCrosses>(ccyPair);

	mlc::util::Nullable<std::string> FXCurrencyUtils::getFwdCrossCurrency(std::string const &ccyPair)
		FXCacheUtils::getCurrencyPair(ccyPair, ccyPairObject);
		return getFwdCrossCurrency(get_value(ccyPairObject));


# Special Currency


isoCode -> mapCcy

	ml::common::util::Nullable<std::string> FXCurrencyUtils::getMapCcyFromSpecialCcy(const std::string & specialCcy)
		Currency::Ptr ccyPtr = Cache<Currency>::instance()[Currency::Index::PrimaryKey(specialCcy)];
		return (*(ccyPtr->getMapCcy())).toString();

mapCcy -> isoCode

	ml::common::util::Nullable<std::string> FXCurrencyUtils::getSpecialCcy(const std::string & normalCcy,
			const Currency::CurrencyType type)
		Currency::SearchCriteria criteria;

		criteria.type = type;
		criteria.isValid = 1;
		criteria.mapCcy = ml::panther::serialize::types::String(normalCcy);

		Currency::PtrList values;
		Cache<Currency>::instance().values(values, criteria);

		return (values[0])->getIsoCode().toString();

isoCode -> mapCcy | isoCode

	std::string FXCurrencyUtils::checkAgainstExistingInternalFormatOrConvertCcy(const std::string & ccy)
		const Currency::Ptr &cachedCcy = Cache<Currency>::instance().get(ccy);
		if ( cachedCcy && !cachedCcy->getMapCcy().isNull() )
			return cachedCcy->getMapCcy()->toString();
		else
			return ccy;


All of matching mapCcy

	bool FXCurrencyUtils::getAllSpecialCcyGivenDirectCcy(const std::string & normalCcy, ml::spec::fx::common::Currency::PtrList& values)
		ml::spec::fx::common::Currency::SearchCriteria criteria;
		criteria.isValid=1;
		criteria.mapCcy =ml::panther::serialize::types::String(normalCcy);;
		ml::spec::cache::Cache<Currency>::instance().values(values, criteria);


# Special Currency Pair

	class CurrencyPair version 1.1 apiexposed
		enum PublishableSide 
			BID_ONLY,
			ASK_ONLY,
			BID_AND_ASK

		enum CurrencyPairType 
			NORMAL,
			NDF,
			ROE,
			LVN_SELL,
			LVN_BUY_SELL,
			OST,
			METAL_LONDON,
			METAL_ZURICH

		enum DecimalIncrementType
			TENTH,
			HALF

		primary key (ccy1, ccy2),


	id     ccy1 ccy2 name   spotQuotePrecision fwdQuotePrecision publishableSide pipSize 
	------ ---- ---- ------ ------------------ ----------------- --------------- ------- 
	424366 AR2  DKK  AR2DKK 4                  2                 2               4       
	576001 AUD  AED  AUDAED 4                  2                 2               4       
	543043 AUD  AL2  AUDAL2 2                  2                 2               2       
	375001 BRL  TRY  BRLTRY 5                  2                 2               4       
	535021 CAD  AUD  CADAUD 5                  2                 2               4       
	535022 CAD  EUR  CADEUR 5                  2                 2               4       

	outrightDenomCcy swapDenomCcy spotStdQuotePrecision spotPrecisionDecimalIncr orderCategoryId 
	---------------- ------------ --------------------- ------------------------ --------------- 
	AR2              AR2          4                     1                        (null)          
	AUD              AUD          4                     1                        (null)          
	AUD              AUD          2                     1                        (null)          
	TRY              TRY          4                     0                        4               
	AUD              AUD          4                     0                        1               
	EUR              EUR          4                     0                        1               

	validForOrders mapCcyPair type isReciprocal fwdHigherQuotePrecision maxTenor 
	-------------- ---------- ---- ------------ ----------------------- -------- 
	N              ARSDKK     2    N            2                       (null)   
	Y              (null)     0    N            2                       (null)   
	Y              AUDALL     2    N            2                       (null)   
	N              (null)     0    Y            2                       (null)   
	N              (null)     0    Y            2                       (null)   
	N              (null)     0    Y            2                       (null)   
	

	spotDisplayBasis spotDisplayBigFigureSize 
	---------------- ------------------------ 
	4                2                        
	4                2                        
	2                2                        
	4                2                        
	4                2                        
	4                2                        	


mapCCyPair -> name

	ml::common::util::Nullable<std::string> FXCurrencyUtils::getSpecialCcyPair(const std::string & normalCcyPair,
			const CurrencyPair::CurrencyPairType type)
		CurrencyPair::SearchCriteria criteria;
		criteria.type = type;
		criteria.isValid = 1;
		criteria.mapCcyPair = ml::panther::serialize::types::String(normalCcyPair);
		CurrencyPair::PtrList values;
		Cache<CurrencyPair>::instance().values(values, criteria);
		return (values[0])->getName().toString();


name -> mapCcyPair

	ml::common::util::Nullable<std::string> FXCurrencyUtils::getMapCcyPairFromSpecialCcyPair(const std::string & specialCcyPair)
		CurrencyPair::Ptr ccyPairPtr = Cache<CurrencyPair>::instance()[CurrencyPair::Index::name(specialCcyPair) ];
		return (*(ccyPairPtr->getMapCcyPair())).toString();


name -> [mapCcyPair | name]


	std::string FXCurrencyUtils::checkAgainstExistingInternalFormatOrConvertCcyPair(const std::string & ccyPair)
		std::string tempCcyPair = ccyPair;
		bool reformat = false;
		std::string::size_type found = tempCcyPair.find('/');
		if(found!=std::string::npos) {
			tempCcyPair.erase(found, 1);
			reformat = true;
		}
		CurrencyPair::SearchCriteria criteria1;
		criteria1.name = ml::panther::serialize::types::String(tempCcyPair);
		CurrencyPair::PtrList values;
		Cache<CurrencyPair>::instance().values(values, criteria1);
		if ( !values.empty())
		{
			Nullable<ml::panther::serialize::types::String> mapCcyPair = (values[0])->getMapCcyPair();
			if(mapCcyPair.isNull())
				if(reformat)
					return (values[0])->getCcy1().toString() + "/" + (values[0])->getCcy2().toString();
				else
					return (values[0])->getName().toString();
			else
				if(reformat)
					ml::fx::common::CurrencyPair newCcypair(mapCcyPair->toString());
					return newCcypair.baseCcy() + "/" + newCcypair.termsCcy();
				else
					return mapCcyPair->toString();
		}
		return ccyPair;



# Currency Type

	bool FXCurrencyUtils::isCurrencyOfType(const std::string & ccy, const Currency::CurrencyType type)
		Currency::Ptr ccyPtr = Cache<Currency>::instance()[Currency::Index::PrimaryKey(ccy)];
		return isCurrencyOfType(ccyPtr, type);

	bool FXCurrencyUtils::isCurrencyOfType(const Currency::Ptr & ccyPtr, const Currency::CurrencyType type)
		return ( ccyPtr &&  ccyPtr->getIsValid() && ccyPtr->getType() == type );

	bool FXCurrencyUtils::isCurrencyROEorLVN(const std::string& ccy)
		Currency::Ptr ccyPtr = Cache<Currency>::instance()[Currency::Index::PrimaryKey(ccy)];
		return isCurrencyOfType(ccyPtr, Currency::ROE) ||
			   isCurrencyOfType(ccyPtr, Currency::LVN_BUY_SELL) ||
			   isCurrencyOfType(ccyPtr, Currency::LVN_SELL);

	bool FXCurrencyUtils::isCurrencySpecial(const std::string & ccy)
		Currency::Ptr ccyPtr = Cache<Currency>::instance()[Currency::Index::PrimaryKey(ccy)];
		return ( ccyPtr &&  ccyPtr->getIsValid() &&	ccyPtr->getType() != Currency::NORMAL );


# Currency Pair Type

	bool FXCurrencyUtils::isCurrencyPairOfType(const std::string& ccyPair, const CurrencyPair::CurrencyPairType type)
		CurrencyPair::Ptr ccyPairPtr = Cache<CurrencyPair>::instance()[CurrencyPair::Index::name(ccyPair) ];
		return isCurrencyPairOfType(ccyPairPtr, type);

	bool FXCurrencyUtils::isCurrencyPairOfType(const CurrencyPair::Ptr& ccyPairPtr, const CurrencyPair::CurrencyPairType type)
		return ( ccyPairPtr && ccyPairPtr->getIsValid() && ccyPairPtr->getType() == type );

	bool FXCurrencyUtils::isCurrencyPairROEorLVN(const std::string& ccyPair)
		CurrencyPair::Ptr ccyPairPtr = Cache<CurrencyPair>::instance()[CurrencyPair::Index::name(ccyPair) ];
		return isCurrencyPairOfType(ccyPairPtr, CurrencyPair::ROE) ||
			   isCurrencyPairOfType(ccyPairPtr, CurrencyPair::LVN_BUY_SELL) ||
			   isCurrencyPairOfType(ccyPairPtr, CurrencyPair::LVN_SELL);

	bool FXCurrencyUtils::isCurrencyPairSpecial(const std::string& ccyPair)
		CurrencyPair::Ptr ccyPairPtr = Cache<CurrencyPair>::instance()[CurrencyPair::Index::name(ccyPair) ];
		return ( ccyPairPtr &&  ccyPairPtr->getIsValid() &&	ccyPairPtr->getType() != CurrencyPair::NORMAL );


Supported if (name && NORMAL) or mapCcy

	bool FXCurrencyUtils::isSupportedCcyPair(const std::string & externalCcypair)
		CurrencyPair::SearchCriteria criteria;
		criteria.mapCcyPair = ml::panther::serialize::types::String(externalCcypair);
		criteria.isValid = 1;

		CurrencyPair::PtrList values;
		Cache<CurrencyPair>::instance().values(values, criteria);

		if (!values.empty())
			return true;
		else
			CurrencyPair::Ptr ccyPair = Cache<CurrencyPair>::instance().get(CurrencyPair::Index::name(externalCcypair));
			return ccyPair && ccyPair->getType() == CurrencyPair::NORMAL;


# Side

	bool FXCurrencyUtils::isBuySideSupported(const std::string& ccyPair)
		// if normal pair, OK
		if ( !isCurrencyPairSpecial(ccyPair) ||
				!isCurrencyPairROEorLVN(ccyPair) )
			return true;

		const std::string ccy1 = ccyPair.substr(0, 3);
		const std::string ccy2 = ccyPair.substr(3, 3);
		return isCurrencyROEorLVN(ccy1)) || isCurrencyOfType(ccy2, Currency::LVN_BUY_SELL);


	bool FXCurrencyUtils::isSellSideSupported(const std::string& ccyPair)
		if ( !isCurrencyPairSpecial(ccyPair) ||
				!isCurrencyPairROEorLVN(ccyPair) )
			return true;

		const std::string ccy1 = ccyPair.substr(0, 3);
		const std::string ccy2 = ccyPair.substr(3, 3);
		return isCurrencyOfType(ccy1, Currency::LVN_BUY_SELL) || isCurrencyROEorLVN(ccy2))


	template <typename CcyOrCcyPairType>
	static std::string getReadableSuffixByType(const CcyOrCcyPairType &obj)
		switch (obj.getType())
			case CcyOrCcyPairType::NDF:
				return "NDF";
			case CcyOrCcyPairType::ROE:
				return "ROE";
			case CcyOrCcyPairType::LVN_SELL:
			case CcyOrCcyPairType::LVN_BUY_SELL:
				return "LVN";
			case CcyOrCcyPairType::OST:
				return "OST";
			default:
				return "";

	std::string FXCurrencyUtils::formatCcy(const std::string& ccy)
	{
		Currency::Ptr ccyPtr =
				Cache<Currency>::instance()[Currency::Index::PrimaryKey(ccy)];

		if ( ccyPtr
				&& ccyPtr->getType() != Currency::NORMAL
				&& !ccyPtr->getMapCcy().isNull() )
		{
			const std::string mapCcy = ccyPtr->getMapCcy()->str();
			return mapCcy + " " + getReadableSuffixByType(*ccyPtr);
		}

		return ccy;
	}

	std::string FXCurrencyUtils::formatCcyPair(const std::string& ccyPair)
	{
		CurrencyPair::Ptr ccyPairPtr =
				Cache<CurrencyPair>::instance()[CurrencyPair::Index::name(ccyPair) ];

		if ( ccyPairPtr
				&& ccyPairPtr->getType() != CurrencyPair::NORMAL
				&& !ccyPairPtr->getMapCcyPair().isNull() )
		{
			const std::string mapCcyPair = ccyPairPtr->getMapCcyPair()->str();
			return mapCcyPair + " " + getReadableSuffixByType(*ccyPairPtr);
		}

		return ccyPair;
	}

	bool FXCurrencyUtils::isInvertedCurrencyPair(const ml::spec::fx::common::CurrencyPair& ccyPair)
	{
	   return ccyPair.getIsReciprocal();
	}

	bool FXCurrencyUtils::isInvertedCurrencyPair(const std::string& currencyPair)
	{
	   const CurrencyPair::Ptr ccyPair = Cache<CurrencyPair>::instance()[CurrencyPair::Index::name(currencyPair) ];
	   if (!ccyPair)
	   {
		  LWARN("CurrencyPair[%s] not found in CurrencyPair cache", currencyPair.c_str());
		  return false;
	   }
	   return ccyPair->getIsReciprocal();
	}

	std::string FXCurrencyUtils::getMarketConventionCurrencyPair(const ml::spec::fx::common::CurrencyPair& ccyPair)
	{
	   if (isInvertedCurrencyPair(ccyPair))
		  return ccyPair.getCcy2().str() + ccyPair.getCcy1().str();
	   return ccyPair.getCcy1().str() + ccyPair.getCcy2().str();
	}

	std::string FXCurrencyUtils::getMarketConventionCurrencyPair(const std::string& currencyPair)
	{
	   const CurrencyPair::Ptr ccyPairPtr = Cache<CurrencyPair>::instance()[CurrencyPair::Index::name(currencyPair) ];
	   return ccyPairPtr ? getMarketConventionCurrencyPair(*ccyPairPtr) : currencyPair;
	}

	std::string FXCurrencyUtils::getCcyIsoCode(const std::string &ccy)
	{
		const Nullable<std::string> mapCcy = getMapCcyFromSpecialCcy(ccy);
		return mapCcy.isNull() ? ccy : *mapCcy;
	}

	std::string FXCurrencyUtils::getCcyPairIsoCode(const std::string &ccyPair)
	{
		const Nullable<std::string> mapCcyPair = getMapCcyPairFromSpecialCcyPair(ccyPair);
		return mapCcyPair.isNull() ? ccyPair : *mapCcyPair;
	}

	Currency::PtrList
	FXCurrencyUtils::getAllVariantsForGivenMapCcy(const std::string & mapCcy)
	{
		Currency::PtrList entries;

		Currency::SearchCriteria criteria;
		criteria.mapCcy = Nullable<ml::panther::serialize::types::String>(mapCcy);
		criteria.isValid = 1;

		Cache<Currency>::instance().values(entries, criteria);
		if ( entries.size() > 0 ) return entries;

		return Currency::PtrList();
	}
