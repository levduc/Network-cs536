int32_t dropsendto(int32_t totalNum, int32_t lossNum)
{
	if((lossNum%totalNum == 0) && (lossNum!=0))
	{
		return 1;
	}
	return -1;
}