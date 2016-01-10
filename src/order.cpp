#include "order.h"
#include "impute.h"
SEXP order(SEXP mpcrossLG_sexp, SEXP groupsToOrder_sexp)
{
	Rcpp::S4 mpcrossLG;
	try
	{
		mpcrossLG = mpcrossLG_sexp;
	}
	catch(...)
	{
		throw std::runtime_error("Input mpcrossLG must be an S4 object");
	}

	Rcpp::S4 lg;
	try
	{
		lg = Rcpp::as<Rcpp::S4>(mpcrossLG.slot("lg"));
	}
	catch(...)
	{
		throw std::runtime_error("Slot mpcrossLG@lg must be an S4 object");
	}

	Rcpp::S4 rf;
	try
	{
		rf = Rcpp::as<Rcpp::S4>(mpcrossLG.slot("rf"));
	}
	catch(...)
	{
		throw std::runtime_error("Slot mpcrossLG@rf must be an S4 object");
	}

	Rcpp::S4 theta;
	try
	{
		theta = Rcpp::as<Rcpp::S4>(rf.slot("theta"));
	}
	catch(...)
	{
		throw std::runtime_error("Slot mpcrossLG@rf@theta must be an S4 object");
	}

	Rcpp::RawVector originalRawData;
	try
	{
		originalRawData = theta.slot("data");
	}
	catch(...)
	{
		throw std::runtime_error("Slot mpcrossLG@rf@theta@data must be a raw vector");
	}

	std::vector<double> levels;
	try
	{
		levels = Rcpp::as<std::vector<double> >(Rcpp::as<Rcpp::NumericVector>(theta.slot("levels")));
	}
	catch(...)
	{
		throw std::runtime_error("Slot mpcrossLG@rf@theta@levels must be a numeric vector");
	}

	std::vector<int> groups;
	try
	{
		groups = Rcpp::as<std::vector<int> >(lg.slot("groups"));
	}
	catch(...)
	{
		throw std::runtime_error("Slot mpcrossLG@lg@groups must be an integer vector");
	}

	std::vector<int> allGroups;
	try
	{
		allGroups = Rcpp::as<std::vector<int> >(lg.slot("allGroups"));
	}
	catch(...)
	{
		throw std::runtime_error("Slot mpcrossLG@lg@allGroups must be an integer vector");
	}

	std::vector<int> groupsToOrder;
	try
	{
		groupsToOrder = Rcpp::as<std::vector<int> >(groupsToOrder_sexp);
	}
	catch(...)
	{
		throw std::runtime_error("Input groupsToOrder must be an integerVector");
	}

	//Check that groupsToOrder contains unique values, which are contained in allGroups
	std::sort(groupsToOrder.begin(), groupsToOrder.end());
	std::sort(allGroups.begin(), allGroups.end());
	if(std::unique(groupsToOrder.begin(), groupsToOrder.end()) != groupsToOrder.end())
	{
		throw std::runtime_error("Input groupsToOrder must contain unique values");
	}
	for(std::vector<int>::iterator groupToOrder = groupsToOrder.begin(); groupToOrder != groupsToOrder.end(); groupToOrder++)
	{
		std::vector<int>::iterator bound = std::lower_bound(allGroups.begin(), allGroups.end(), *groupToOrder);
		if(bound == allGroups.end() || *bound != *groupToOrder)
		{
			throw std::runtime_error("Input groupsToOrder contained a group that was not present in slot mpcrossLG@lg@allGroups");
		}
	}


	Rcpp::Function asDist("as.dist"), seriate("seriate"), getOrder("get_order"), criterion("criterion");
	int nMarkers = groups.size();

	std::vector<int> permutation, currentGroupPermutation;
	permutation.reserve(nMarkers);
	std::vector<int> markersThisGroup;
	//This holds a copy of the raw data, for the purposes of doing imputation. We don't want to touch the original, obviously. 
	std::vector<unsigned char> imputedRaw;
	for(std::vector<int>::iterator currentGroup = allGroups.begin(); currentGroup != allGroups.end(); currentGroup++)
	{
		markersThisGroup.clear();
		//linkage groups are not required to be contiguous in this case, so we have to scan through to find the number of markers in this group
		for(std::vector<int>::iterator currentMarkerGroup = groups.begin(); currentMarkerGroup != groups.end(); currentMarkerGroup++)
		{
			if(*currentMarkerGroup == *currentGroup)
			{
				markersThisGroup.push_back((int)std::distance(groups.begin(), currentMarkerGroup));
			}
		}
		int nMarkersCurrentGroup = (int)markersThisGroup.size();
		if(nMarkersCurrentGroup == 0) continue;
		
		//Do imputation
		//So first make a copy of the raw data subset
		imputedRaw.resize((nMarkersCurrentGroup * (nMarkersCurrentGroup + 1)) / 2);
		//column
		for(int i = 0; i < nMarkersCurrentGroup; i++)
		{
			//row
			for(int j = 0; j <= i; j++)
			{
				int row = markersThisGroup[j], column = markersThisGroup[i];
				if(row > column) std::swap(row, column);
				imputedRaw[(i * (i + 1))/2 + j] = originalRawData[(column * (column + 1))/2 + row];
			}
		}
		//For the purposes of imputation the markers to be imputed are 0 to nMarkersCurrentGroup-1
		std::vector<int> contiguousIndices(nMarkersCurrentGroup);
		for(int i = 0; i < nMarkersCurrentGroup; i++) contiguousIndices[i] = i;
		std::string error;
		bool imputationResult = impute(&(imputedRaw[0]), levels, NULL, NULL, contiguousIndices, error);
		if(!imputationResult)
		{
			throw std::runtime_error(error.c_str());
		}

		//Form the numeric matrix for ordering. 
		Rcpp::NumericMatrix subMatrix(nMarkersCurrentGroup, nMarkersCurrentGroup);
		double* mem = &(subMatrix(0,0));
		//Determine whether or not the submatrix is zero, as seriation seems to screw up for all-zero matrices.
		bool nonZero = false;
		for(int i = 0; i < nMarkersCurrentGroup; i++)
		{
			for(int j = 0; j < nMarkersCurrentGroup; j++)
			{
				double* dest = mem + i + j * nMarkersCurrentGroup;
				int row = i, column = j;
				if(row > column) std::swap(row, column);
				unsigned char rawValue = imputedRaw[(column * (column + 1))/2 + row];
				if(rawValue == 0xff) throw std::runtime_error("Internal error: NA value found when constructing matrix to order");
				*dest = levels[rawValue];
				nonZero |= (*dest != 0);
			}
		}
		if(nonZero)
		{
			Rcpp::RObject distSubMatrix = asDist(Rcpp::Named("m") = subMatrix);
			
			Rcpp::List nRepsControl = Rcpp::List::create(Rcpp::Named("nreps") = 5);
			Rcpp::RObject result = seriate(Rcpp::Named("x") = distSubMatrix, Rcpp::Named("method") = "ARSA", Rcpp::Named("control") = nRepsControl);
			std::vector<int> arsaResult = Rcpp::as<std::vector<int> >(getOrder(result));
			double arsaCriterionResult = Rcpp::as<double>(criterion(Rcpp::Named("x") = distSubMatrix, Rcpp::Named("order") = result, Rcpp::Named("method") = "AR_events"));

			//now consider identity permutation
			double identityCriterionResult = Rcpp::as<double>(criterion(Rcpp::Named("x") = distSubMatrix, Rcpp::Named("method") = "AR_events"));

			if(arsaCriterionResult < identityCriterionResult)
			{
				currentGroupPermutation.swap(arsaResult);
			}
			else
			{
				std::vector<int> identity;
				for(int i = 0; i < nMarkersCurrentGroup; i++) identity.push_back(i+1);
				currentGroupPermutation.swap(identity);
			}
			for(int i = 0; i < nMarkersCurrentGroup; i++) permutation.push_back(markersThisGroup[currentGroupPermutation[i]-1]+1);
		}
		else
		{
			for(int i = 0; i < nMarkersCurrentGroup; i++) permutation.push_back(markersThisGroup[i]+1);
		}
	}
	return Rcpp::wrap(permutation);
}