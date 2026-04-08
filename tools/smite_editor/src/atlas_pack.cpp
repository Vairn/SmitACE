#include "atlas_pack.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

AtlasPacker::AtlasNode *AtlasPacker::packInto(AtlasPacker::AtlasNode *n, int rw, int rh)
{
	if (!n)
		return nullptr;
	if (n->left) {
		AtlasPacker::AtlasNode *a = packInto(n->left.get(), rw, rh);
		if (a)
			return a;
		return packInto(n->right.get(), rw, rh);
	}
	if (n->filled || n->rect.w < rw || n->rect.h < rh)
		return nullptr;
	if (n->rect.w == rw && n->rect.h == rh) {
		n->filled = true;
		return n;
	}
	if ((n->rect.w - rw) > (n->rect.h - rh)) {
		n->left = std::make_unique<AtlasNode>();
		n->left->rect = {n->rect.x, n->rect.y, rw, n->rect.h};
		n->right = std::make_unique<AtlasNode>();
		n->right->rect = {n->rect.x + rw, n->rect.y, n->rect.w - rw, n->rect.h};
	} else {
		n->left = std::make_unique<AtlasNode>();
		n->left->rect = {n->rect.x, n->rect.y, n->rect.w, rh};
		n->right = std::make_unique<AtlasNode>();
		n->right->rect = {n->rect.x, n->rect.y + rh, n->rect.w, n->rect.h - rh};
	}
	return packInto(n->left.get(), rw, rh);
}

void AtlasPacker::resizeCanvas(std::vector<unsigned char> &px, int ow, int oh, int nw, int nh)
{
	std::vector<unsigned char> next((size_t)nw * nh * 4);
	for (int y = 0; y < nh; y++) {
		for (int x = 0; x < nw; x++) {
			size_t di = ((size_t)y * nw + x) * 4;
			if (x < ow && y < oh) {
				size_t si = ((size_t)y * ow + x) * 4;
				memcpy(&next[di], &px[si], 4);
			} else {
				next[di + 0] = 0;
				next[di + 1] = 0;
				next[di + 2] = 0;
				next[di + 3] = 255;
			}
		}
	}
	px.swap(next);
}

void AtlasPacker::blitNearest(const std::vector<unsigned char> &src, int sw, int sh, std::vector<unsigned char> &dst, int dw, int dh, int dx, int dy, int canvasW)
{
	int canvasH = (int)dst.size() / (canvasW * 4);
	for (int j = 0; j < dh; j++) {
		int sj = (int)((int64_t)j * sh / dh);
		sj = std::min(sj, sh - 1);
		for (int i = 0; i < dw; i++) {
			int si = (int)((int64_t)i * sw / dw);
			si = std::min(si, sw - 1);
			int di = dx + i;
			int dj = dy + j;
			if (di < 0 || dj < 0 || di >= canvasW || dj >= canvasH)
				continue;
			size_t sidx = ((size_t)sj * sw + si) * 4;
			size_t didx = ((size_t)dj * canvasW + di) * 4;
			dst[didx + 0] = src[sidx + 0];
			dst[didx + 1] = src[sidx + 1];
			dst[didx + 2] = src[sidx + 2];
			dst[didx + 3] = src[sidx + 3];
		}
	}
}

static void clearPixels(std::vector<unsigned char> &px, int w, int h, unsigned char r, unsigned char g, unsigned char b)
{
	px.assign((size_t)w * h * 4, 0);
	for (int i = 0; i < w * h; i++) {
		px[i * 4 + 0] = r;
		px[i * 4 + 1] = g;
		px[i * 4 + 2] = b;
		px[i * 4 + 3] = 255;
	}
}

void AtlasPacker::reset(int startW, int startH)
{
	root_ = std::make_unique<AtlasNode>();
	root_->rect = {0, 0, startW, startH};
	cw_ = startW;
	ch_ = startH;
	clearPixels(pixels_, cw_, ch_, 0, 0, 0);
	packed_.clear();
}

void AtlasPacker::expandFor(int rw, int rh)
{
	int ow = cw_, oh = ch_;
	std::unique_ptr<AtlasNode> oldRoot = std::move(root_);
	int nw, nh;
	std::unique_ptr<AtlasNode> rightNode;
	if (ow < oh) {
		nw = ow + rw;
		nh = oh;
		rightNode = std::make_unique<AtlasNode>();
		rightNode->rect = {ow, 0, rw, oh};
	} else {
		nw = ow;
		nh = oh + rh;
		rightNode = std::make_unique<AtlasNode>();
		rightNode->rect = {0, oh, ow, rh};
	}
	auto newRoot = std::make_unique<AtlasNode>();
	newRoot->rect = {0, 0, nw, nh};
	newRoot->left = std::move(oldRoot);
	newRoot->right = std::move(rightNode);
	root_ = std::move(newRoot);
	resizeCanvas(pixels_, ow, oh, nw, nh);
	cw_ = nw;
	ch_ = nh;
}

bool AtlasPacker::addImage(const std::vector<unsigned char> &rgba, int srcW, int srcH, const std::string &name, int gutter, std::string &err)
{
	if (srcW < 1 || srcH < 1 || rgba.size() < (size_t)srcW * srcH * 4) {
		err = "bad image";
		return false;
	}
	const int pw = srcW + 2 * gutter;
	const int ph = srcH + 2 * gutter;
	for (;;) {
		AtlasNode *leaf = packInto(root_.get(), pw, ph);
		if (leaf) {
			blitNearest(rgba, srcW, srcH, pixels_, pw, ph, leaf->rect.x, leaf->rect.y, cw_);
			PackedEntry e;
			e.name = name;
			e.x = leaf->rect.x + gutter;
			e.y = leaf->rect.y + gutter;
			e.w = srcW;
			e.h = srcH;
			packed_.push_back(std::move(e));
			return true;
		}
		expandFor(pw, ph);
		if (cw_ > 8192 || ch_ > 8192) {
			err = "atlas too large";
			return false;
		}
	}
}
