#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct PackedEntry {
	std::string name;
	int x = 0, y = 0, w = 0, h = 0;
};

class AtlasPacker {
public:
	void reset(int startW, int startH);
	void expandFor(int rw, int rh);

	// Packs rgba (srcW x srcH) with outer gutter; blits into atlas (nearest scale to padded slot).
	bool addImage(const std::vector<unsigned char> &rgba, int srcW, int srcH, const std::string &name, int gutter, std::string &err);

	int width() const { return cw_; }
	int height() const { return ch_; }
	const std::vector<unsigned char> &pixels() const { return pixels_; }
	const std::vector<PackedEntry> &packed() const { return packed_; }

private:
	struct ApRect {
		int x = 0, y = 0, w = 0, h = 0;
	};
	struct AtlasNode {
		ApRect rect{};
		bool filled = false;
		std::unique_ptr<AtlasNode> left;
		std::unique_ptr<AtlasNode> right;
	};
	AtlasNode *packInto(AtlasNode *n, int rw, int rh);
	static void resizeCanvas(std::vector<unsigned char> &px, int ow, int oh, int nw, int nh);
	static void blitNearest(const std::vector<unsigned char> &src, int sw, int sh, std::vector<unsigned char> &dst, int dw, int dh, int dx, int dy, int canvasW);

	std::unique_ptr<AtlasNode> root_;
	int cw_ = 0, ch_ = 0;
	std::vector<unsigned char> pixels_;
	std::vector<PackedEntry> packed_;
};
