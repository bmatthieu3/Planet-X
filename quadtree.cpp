#include <memory>
#include <set>
#include <unordered_map>
#include <iostream>
#include "quadtree.h"

int Quadtree::max_elements_ = 4;

Quadtree::Rect_t Quadtree::global_rect_ = {0.f, 0.f, 1024.f, 768.f};

static Quadtree::RegisterElementFunc_t element_in_quad_func = nullptr;

void Quadtree::set_element_in_quad_func(const RegisterElementFunc_t& func) {
    element_in_quad_func = func;
}

Quadtree::Quadtree(const QuadtreePtr parent, const uint8_t location) : parent_(parent), num_elements_(0) {
	if(parent_ == nullptr && location == NONE) {
		rect_ = global_rect_;
		return;
	}

	if(parent_ == nullptr) {
		std::cerr << "error : this node has no parent !" << std::endl;
		exit(EXIT_FAILURE);
	}
	
	const Quadtree::Rect_t parent_rect = parent_->rect_;
	const float w = parent_rect.w/2.f;
	const float h = parent_rect.h/2.f;

	switch(location) {
		case TOP_LEFT:
			rect_ = {parent_rect.x, parent_rect.y,
				 w, h};
			break;
		case TOP_RIGHT:
			rect_ = {parent_rect.x + w, parent_rect.y,
				 w, h};
			break;
		case BOTTOM_LEFT:
			rect_ = {parent_rect.x, parent_rect.y + h,
				 w, h};
			break;
		case BOTTOM_RIGHT:
			rect_ = {parent_rect.x + w, parent_rect.y + h,
				 w, h};
			break;
		default:
			break;
	}
}

Quadtree::~Quadtree() {

}

void Quadtree::insert(const EntityPtr element_ptr, std::unordered_map<EntityPtr, std::set<QuadtreePtr>>& quads_map) {
	if(element_in_quad_func(element_ptr, rect_)) {
        num_elements_++;
		if(children_.empty()) {
            if(elements_.size() < max_elements_) {
				if(elements_.empty()) {
					elements_[element_ptr] = std::set<EntityPtr>();
	                quads_map[element_ptr].insert(shared_from_this());
					return;
				}

				for(auto& kv_inserted : elements_) {
					const EntityPtr element_inserted_ptr(kv_inserted.first);

					elements_[element_ptr].insert(element_inserted_ptr);
					elements_[element_inserted_ptr].insert(element_ptr);
				}
				
                quads_map[element_ptr].insert(shared_from_this());
				return;
			}

			children_.push_back(std::make_shared<Quadtree>(shared_from_this(), TOP_LEFT));
			children_.push_back(std::make_shared<Quadtree>(shared_from_this(), TOP_RIGHT));
			children_.push_back(std::make_shared<Quadtree>(shared_from_this(), BOTTOM_LEFT));
			children_.push_back(std::make_shared<Quadtree>(shared_from_this(), BOTTOM_RIGHT));
	        
            for(auto& child_ptr : children_) {
			    for(auto& kv_inserted : elements_) {
				    const EntityPtr element_already_ptr(kv_inserted.first);
                    quads_map[element_already_ptr].erase(shared_from_this());
				    child_ptr->insert(element_already_ptr, quads_map);	
                }
            }

		    if(!elements_.empty())
			    elements_.clear();
    	}
		
        for(auto& child_ptr : children_) {
		    child_ptr->insert(element_ptr, quads_map);
		}
	}
}

void Quadtree::update(std::unordered_map<EntityPtr, std::set<QuadtreePtr>>& quads_map) {

    if(num_elements_ <= max_elements_ && !children_.empty()) {
        std::set<EntityPtr> elts_to_insert; 
        get_all_children_elt(elts_to_insert);
            
        children_.clear();
        num_elements_ = 0;
        
        for(auto& elt_to_insert : elts_to_insert) {
            quads_map[elt_to_insert].clear();
            insert(elt_to_insert, quads_map);
        } 
    }
  
    for(auto& child_ptr : children_) {
	    child_ptr->update(quads_map);
	}
}

void Quadtree::get_all_children_elt(std::set<EntityPtr>& elts) {
    for(auto& elt : elements_) {
        elts.insert(elt.first);
    }

    for(auto& child : children_) {
        child->get_all_children_elt(elts);
    }
}

void Quadtree::decrease_num_elements(const EntityPtr element_ptr) {
    if(is_parent(element_ptr)) {
	    num_elements_--;
        for(auto& child : children_) {
            child->decrease_num_elements(element_ptr);
        }
    }
}

bool Quadtree::is_parent(const EntityPtr element_ptr) {
    for(auto& child : children_) {
        if(child->is_parent(element_ptr)) {
            return true;
        }
    }

    return (elements_.find(element_ptr) != elements_.end());
}

void Quadtree::remove(const EntityPtr element_ptr, std::unordered_map<EntityPtr, std::set<QuadtreePtr>>& quads_map) {
    elements_.erase(element_ptr);

	for(auto& kv_inserted : elements_) {
		kv_inserted.second.erase(element_ptr);
	}

    quads_map[element_ptr].erase(shared_from_this());
}


