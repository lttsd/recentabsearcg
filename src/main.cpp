#include <Geode/Geode.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/ui/TextInput.hpp>

using namespace geode::prelude;

class $modify(RecentSearchLayer, LevelBrowserLayer) {
	struct Fields {
		TextInput* input = nullptr;
		CCMenuItemSpriteExtra* stopBtn = nullptr;
		CCLabelBMFont* status = nullptr;
		Ref<CCArray> found;
		Ref<GJSearchObject> current;
		std::string query;
		std::string key;
		int page = 0;
		int maxPages = 0;
		bool active = false;
	};

	bool init(GJSearchObject* object) {
		if (!LevelBrowserLayer::init(object)) return false;
		if (object->m_searchType != SearchType::Recent) return true;

		auto fields = m_fields.self();
		auto size = CCDirector::get()->getWinSize();

		fields->input = TextInput::create(200.f, "Search in Recent");
		fields->input->setPosition({size.width / 2 - 50.f, 18.f});
		fields->input->setScale(0.7f);
		fields->input->setMaxCharCount(30);
		fields->input->setID("search-input"_spr);
		this->addChild(fields->input, 10);

		fields->status = CCLabelBMFont::create("", "goldFont.fnt");
		fields->status->setScale(0.5f);
		fields->status->setPosition({size.width / 2, 40.f});
		fields->status->setID("search-status"_spr);
		this->addChild(fields->status, 10);

		auto menu = CCMenu::create();
		menu->setPosition({0.f, 0.f});
		menu->setID("search-menu"_spr);

		auto find = CCMenuItemSpriteExtra::create(ButtonSprite::create("Search"), this, menu_selector(RecentSearchLayer::onFind));
		find->setPosition({size.width / 2 + 80.f, 18.f});
		find->setID("search-button"_spr);
		menu->addChild(find);

		fields->stopBtn = CCMenuItemSpriteExtra::create(ButtonSprite::create("Stop"), this, menu_selector(RecentSearchLayer::onStop));
		fields->stopBtn->setPosition({size.width / 2 + 150.f, 18.f});
		fields->stopBtn->setVisible(false);
		fields->stopBtn->setID("stop-button"_spr);
		menu->addChild(fields->stopBtn);

		this->addChild(menu, 10);
		return true;
	}

	void onFind(CCObject*) {
		auto fields = m_fields.self();
		if (fields->active) return;

		auto query = utils::string::toLower(utils::string::trim(fields->input->getString()));
		if (query.empty()) {
			fields->status->setString("");
			this->loadPage(m_searchObject);
			return;
		}

		fields->query = query;
		fields->found = CCArray::create();
		fields->page = 0;
		fields->maxPages = static_cast<int>(Mod::get()->getSettingValue<int64_t>("max-pages"));
		fields->active = true;
		fields->stopBtn->setVisible(true);
		this->requestPage();
	}

	void onStop(CCObject*) {
		if (m_fields->active) this->finish();
	}

	void requestPage() {
		auto fields = m_fields.self();
		fields->current = m_searchObject->getPageObject(fields->page);
		fields->key = fields->current->getKey();
		fields->status->setString(fmt::format("Page {}/{} - {} found", fields->page + 1, fields->maxPages, fields->found->count()).c_str());

		auto glm = GameLevelManager::sharedState();
		glm->m_levelManagerDelegate = this;
		if (auto cached = glm->getStoredOnlineLevels(fields->key.c_str())) {
			this->handlePage(cached);
		} else {
			glm->getOnlineLevels(fields->current);
		}
	}

	void handlePage(CCArray* levels) {
		auto fields = m_fields.self();
		int count = levels ? levels->count() : 0;
		if (count > 0) {
			for (auto level : CCArrayExt<GJGameLevel*>(levels)) {
				auto name = utils::string::toLower(level->m_levelName.c_str());
				if (name.find(fields->query) != std::string::npos) fields->found->addObject(level);
			}
		}

		fields->page++;
		if (count == 0 || fields->page >= fields->maxPages) this->finish();
		else this->requestPage();
	}

	void finish() {
		auto fields = m_fields.self();
		fields->active = false;
		fields->stopBtn->setVisible(false);

		int count = fields->found->count();
		fields->status->setString(fmt::format("{} found in {} pages", count, fields->page).c_str());

		m_itemCount = count;
		m_pageStartIdx = 0;
		m_pageEndIdx = count;
		this->setupLevelBrowser(fields->found);
		this->updateLevelsLabel();
	}

	void loadLevelsFinished(CCArray* levels, char const* key, int type) {
		auto fields = m_fields.self();
		if (!fields->active) return LevelBrowserLayer::loadLevelsFinished(levels, key, type);
		if (key && fields->key == key) this->handlePage(levels);
	}

	void loadLevelsFailed(char const* key, int type) {
		auto fields = m_fields.self();
		if (!fields->active) return LevelBrowserLayer::loadLevelsFailed(key, type);
		if (key && fields->key == key) this->finish();
	}

	void setupPageInfo(gd::string info, char const* key) {
		if (!m_fields->active) LevelBrowserLayer::setupPageInfo(info, key);
	}
};
