	void applyFunnel(int startPosition, int endPosition, int finalCounter, int funnelID)
	{
		if(errorProb == 0)
		{
			applyFunnelNoError(startPosition, endPosition, finalCounter, funnelID);
		}
		else
		{
			applyFunnelWithError(startPosition, endPosition, finalCounter, funnelID);
		}
	}
	void applyFunnelNoError(int startPosition, int endPosition, int finalCounter, int funnelID)
	{
		if(logFunnelHaplotypeProbabilities == NULL || errorProb != errorProb || errorProb != 0)
		{
			throw std::runtime_error("Internal error");
		}
		
		//Initialise the algorithm. For infinite generations of selfing, we don't need to bother with the hetData object, as there are no hets
		funnelEncoding enc = (*lineFunnelEncodings)[(*lineFunnelIDs)[finalCounter]];
		int funnel[16];
		for(int founderCounter = 0; founderCounter < nFounders; founderCounter++)
		{
			funnel[founderCounter] = ((enc & (15 << (4*founderCounter))) >> (4*founderCounter));
		}
		int startMarkerIndex = allPositions.markerIndices[startPosition];

		//Is the first marker just a placeholder - A place where we want to impute, but for which there is no actual data?
		if(startMarkerIndex == -1)
		{
			for(int founderCounter = 0; founderCounter < nFounders; founderCounter++)
			{
				intermediate1(founderCounter, 0) = intermediate2(founderCounter, 0) = founderCounter+1;
				pathLengths2[founderCounter] = pathLengths1[founderCounter] = 0;
			}
		}
		else
		{
			int markerValue = recodedFinals(finalCounter, startPosition);
			for(int founderCounter = 0; founderCounter < nFounders; founderCounter++)
			{
				intermediate1(founderCounter, 0) = intermediate2(founderCounter, 0) = founderCounter+1;
				pathLengths2[founderCounter] = pathLengths1[founderCounter] = 0;
				if(recodedFounders(founderCounter, startMarkerIndex) != markerValue && markerValue != NA_INTEGER)
				{
					pathLengths2[founderCounter] = pathLengths1[founderCounter] = -std::numeric_limits<double>::infinity();
				}
			}
		}

		int identicalIndex = 0;
		for(int positionCounter = startPosition; positionCounter < endPosition - 1; positionCounter++)
		{
			int markerIndex = allPositions.markerIndices[positionCounter+1];
			if(markerIndex == -1)
			{
				//The founder at the next marker
				for(int founderCounter = 0; founderCounter < nFounders; founderCounter++)
				{
					//Founder at the previous marker. 
					std::fill(working.begin(), working.end(), -std::numeric_limits<double>::infinity());
					for(int founderCounter2 = 0; founderCounter2 < nFounders; founderCounter2++)
					{
						working[funnel[founderCounter2]] = pathLengths1[funnel[founderCounter2]] + (*logFunnelHaplotypeProbabilities)(positionCounter-startPosition, 0).values[founderCounter2][founderCounter];
					}
					//Get the shortest one, and check that it's not negative infinity.
					std::vector<double>::iterator longest = std::max_element(working.begin(), working.end());
					int bestPrevious = (int)std::distance(working.begin(), longest);
					
					memcpy(&(intermediate2(funnel[founderCounter], identicalIndex)), &(intermediate1(bestPrevious, identicalIndex)), sizeof(int)*(positionCounter - startPosition + 1 - identicalIndex));
					intermediate2(funnel[founderCounter], positionCounter-startPosition+1) = funnel[founderCounter]+1;
					pathLengths2[funnel[founderCounter]] = *longest;
				}
			}
			else
			{
				int markerValue = recodedFinals(finalCounter, markerIndex);
				//The founder at the next marker
				for(int founderCounter = 0; founderCounter < nFounders; founderCounter++)
				{
					if(recodedFounders(funnel[founderCounter], markerIndex) == markerValue || markerValue == NA_INTEGER)
					{
						//Founder at the previous marker. 
						std::fill(working.begin(), working.end(), -std::numeric_limits<double>::infinity());
						for(int founderCounter2 = 0; founderCounter2 < nFounders; founderCounter2++)
						{
							working[funnel[founderCounter2]] = pathLengths1[funnel[founderCounter2]] + (*logFunnelHaplotypeProbabilities)(positionCounter-startPosition, 0).values[founderCounter2][founderCounter];
						}
						//Get the shortest one, and check that it's not negative infinity.
						std::vector<double>::iterator longest = std::max_element(working.begin(), working.end());
						int bestPrevious = (int)std::distance(working.begin(), longest);
						
						memcpy(&(intermediate2(funnel[founderCounter], identicalIndex)), &(intermediate1(bestPrevious, identicalIndex)), sizeof(int)*(positionCounter - startPosition + 1 - identicalIndex));
						intermediate2(funnel[founderCounter], positionCounter-startPosition+1) = funnel[founderCounter]+1;
						pathLengths2[funnel[founderCounter]] = *longest;
					}
					else
					{
						pathLengths2[funnel[founderCounter]] = -std::numeric_limits<double>::infinity();
					}
				}
			}
			//If this condition throws, it's almost guaranteed to be because the map contains two markers at the same location, but the data implies a non-zero distance because recombinations are observed to occur between them.
			std::vector<double>::iterator longest = std::max_element(pathLengths2.begin(), pathLengths2.end());
			if(*longest == -std::numeric_limits<double>::infinity()) throw impossibleDataException(positionCounter, finalCounter);

			intermediate1.swap(intermediate2);
			pathLengths1.swap(pathLengths2);
			while(identicalIndex != positionCounter-startPosition+ 1)
			{
				int value = intermediate1(0, identicalIndex);
				for(int founderCounter = 1; founderCounter < nFounders; founderCounter++)
				{
					if(value != intermediate1(founderCounter, identicalIndex)) goto stopIdenticalSearch;
				}
				for(int founderCounter = 0; founderCounter < nFounders; founderCounter++)
				{
					intermediate2(founderCounter, identicalIndex) = value;
				}
				identicalIndex++;
			}
stopIdenticalSearch:
			;
		}
	}
	void applyFunnelWithError(int startPosition, int endPosition, int finalCounter, int funnelID)
	{
		if(logFunnelHaplotypeProbabilities == NULL || errorProb != errorProb || errorProb <= 0 || errorProb >= 1)
		{
			throw std::runtime_error("Internal error");
		}
		//Initialise the algorithm. For infinite generations of selfing, we don't need to bother with the hetData object, as there are no hets
		funnelEncoding enc = (*lineFunnelEncodings)[(*lineFunnelIDs)[finalCounter]];
		int funnel[16];
		for(int founderCounter = 0; founderCounter < nFounders; founderCounter++)
		{
			funnel[founderCounter] = ((enc & (15 << (4*founderCounter))) >> (4*founderCounter));
		}
		int startMarkerIndex = allPositions.markerIndices[startPosition];
		//Is the first marker just a placeholder - A place where we want to impute, but for which there is no actual data?
		if(startMarkerIndex == -1)
		{
			for(int founderCounter = 0; founderCounter < nFounders; founderCounter++)
			{
				intermediate1(founderCounter, 0) = intermediate2(founderCounter, 0) = founderCounter+1;
				pathLengths2[founderCounter] = pathLengths1[founderCounter] = log(1.0 / (double)nFounders);
			}
		}
		else
		{
			int markerValue = recodedFinals(finalCounter, startMarkerIndex);
			::markerData& startMarkerData = markerData.allMarkerPatterns[markerData.markerPatternIDs[startMarkerIndex]];
			for(int founderCounter = 0; founderCounter < nFounders; founderCounter++)
			{
				intermediate1(founderCounter, 0) = intermediate2(founderCounter, 0) = founderCounter+1;
				pathLengths2[founderCounter] = pathLengths1[founderCounter] = 0;
				if(recodedFounders(founderCounter, startPosition) == markerValue)
				{
					pathLengths2[founderCounter] = pathLengths1[founderCounter] = log((1.0 / (double)nFounders) * ((1 - errorProb) + errorProb / (double)startMarkerData.nObservedValues));
				}
				else if(markerValue == NA_INTEGER)
				{
					pathLengths2[founderCounter] = pathLengths1[founderCounter] = log(1.0 / (double)nFounders);
				}
				else pathLengths2[founderCounter] = pathLengths1[founderCounter] = log((1.0 / (double)nFounders) * errorProb / (double)startMarkerData.nObservedValues);
			}
		}
		int identicalIndex = 0;
		for(int positionCounter = startPosition; positionCounter < endPosition - 1; positionCounter++)
		{
			int markerIndex = allPositions.markerIndices[positionCounter+1];
			if(markerIndex == -1)
			{
				for(int founderCounter = 0; founderCounter < nFounders; founderCounter++)
				{
					//Founder at the previous marker. 
					std::fill(working.begin(), working.end(), -std::numeric_limits<double>::infinity());
					for(int founderCounter2 = 0; founderCounter2 < nFounders; founderCounter2++)
					{
						working[funnel[founderCounter2]] = pathLengths1[funnel[founderCounter2]] + (*logFunnelHaplotypeProbabilities)(positionCounter-startPosition, 0).values[founderCounter2][founderCounter];
					}
					//Get the shortest one, and check that it's not negative infinity.
					std::vector<double>::iterator longest = std::max_element(working.begin(), working.end());
					int bestPrevious = (int)std::distance(working.begin(), longest);
					
					memcpy(&(intermediate2(funnel[founderCounter], identicalIndex)), &(intermediate1(bestPrevious, identicalIndex)), sizeof(int)*(positionCounter - startPosition + 1 - identicalIndex));
					intermediate2(funnel[founderCounter], positionCounter-startPosition+1) = funnel[founderCounter]+1;
					pathLengths2[funnel[founderCounter]] = *longest;
				}
			}
			else
			{
				int markerValue = recodedFinals(finalCounter, markerIndex);
				::markerData& currentMarkerData = markerData.allMarkerPatterns[markerData.markerPatternIDs[markerIndex]];
				//The founder at the next marker
				for(int founderCounter = 0; founderCounter < nFounders; founderCounter++)
				{
					double increment;
					if(recodedFounders(funnel[founderCounter], markerIndex) == markerValue)
					{
						increment = log((1 - errorProb) + errorProb / (double)currentMarkerData.nObservedValues);
					}
					else if(markerValue == NA_INTEGER)
					{
						increment = 0;
					}
					else increment = log(errorProb / (double)currentMarkerData.nObservedValues);
					//Founder at the previous marker. 
					std::fill(working.begin(), working.end(), -std::numeric_limits<double>::infinity());
					for(int founderCounter2 = 0; founderCounter2 < nFounders; founderCounter2++)
					{
						working[funnel[founderCounter2]] = pathLengths1[funnel[founderCounter2]] + (*logFunnelHaplotypeProbabilities)(positionCounter-startPosition, 0).values[founderCounter2][founderCounter] + increment;
					}
					//Get the shortest one, and check that it's not negative infinity.
					std::vector<double>::iterator longest = std::max_element(working.begin(), working.end());
					int bestPrevious = (int)std::distance(working.begin(), longest);
					
					memcpy(&(intermediate2(funnel[founderCounter], identicalIndex)), &(intermediate1(bestPrevious, identicalIndex)), sizeof(int)*(positionCounter- startPosition + 1 - identicalIndex));
					intermediate2(funnel[founderCounter],positionCounter-startPosition+1) = funnel[founderCounter]+1;
					pathLengths2[funnel[founderCounter]] = *longest;
				}
			}
			//If this condition throws, it's almost guaranteed to be because the map contains two markers at the same location, but the data implies a non-zero distance because recombinations are observed to occur between them.
			std::vector<double>::iterator longest = std::max_element(pathLengths2.begin(), pathLengths2.end());
			if(*longest == -std::numeric_limits<double>::infinity()) throw impossibleDataException(positionCounter, finalCounter);

			intermediate1.swap(intermediate2);
			pathLengths1.swap(pathLengths2);
			while(identicalIndex != positionCounter-startPosition + 1)
			{
				int value = intermediate1(0, identicalIndex);
				for(int founderCounter = 1; founderCounter < nFounders; founderCounter++)
				{
					if(value != intermediate1(founderCounter, identicalIndex)) goto stopIdenticalSearch;
				}
				for(int founderCounter = 0; founderCounter < nFounders; founderCounter++)
				{
					intermediate2(founderCounter, identicalIndex) = value;
				}
				identicalIndex++;
			}
stopIdenticalSearch:
			;
		}
	}
