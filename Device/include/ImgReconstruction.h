#pragma once
#include "ImgReconstruction.cuh"
namespace IR {
	inline void init() { ImgReconstruction::getInstance(); }

	inline int setup(size_t max_raw_data_size, int width, int height, int channels, int lineperBlock,int phase){
		return ImgReconstruction::getInstance().setup(max_raw_data_size, width, height, channels, lineperBlock,phase);
	}

	inline int compute(const DataBlock* block, ImageBufferManager::Chunk* chunk,unsigned int line,int phase) {
		return ImgReconstruction::getInstance().compute(block, chunk,line,phase);
	}
	inline int setReconstructionMap(const int* h_pixel_data_counts) {
		return ImgReconstruction::getInstance().setReconstructionMap(h_pixel_data_counts);
	}
}