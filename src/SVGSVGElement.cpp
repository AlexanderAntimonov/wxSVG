//////////////////////////////////////////////////////////////////////////////
// Name:        SVGSVGElement.cpp
// Purpose:     
// Author:      Alex Thuering
// Created:     2005/05/10
// RCS-ID:      $Id: SVGSVGElement.cpp,v 1.13 2017/05/01 17:06:53 ntalex Exp $
// Copyright:   (c) 2005 Alex Thuering
// Licence:     wxWindows licence
//////////////////////////////////////////////////////////////////////////////

#include "SVGSVGElement.h"
#include "SVGStyleElement.h"
#include <wx/tokenzr.h>
#include <wx/log.h>

#include <unordered_map>
#include <string>
#include <regex>

typedef std::unordered_map<wxString, std::shared_ptr<wxCSSStyleDeclaration>> CssStyleMap;
typedef std::unordered_map<wxString, wxSVGElement*, wxStringHash> WxStringToElementMap;

struct wxSVGSVGElement::wxSVGSVGElementPrivate
{
    CssStyleMap cssClasses, cssIds, cssElements;
    WxStringToElementMap getElementByIdCache;
    bool elementByIdWasCached = false;

    void CacheElement(const wxString& elementId, wxSVGElement* element);
    void CacheElementsIds(wxSVGElement* root);
    wxSVGElement* GetElementByIdCached(const wxString& elementId, wxSVGElement* svg);
    wxSVGElement* RecurseElementId(wxSVGElement* root, const wxString& elementId);
};


wxSVGSVGElement::wxSVGSVGElement(wxString tagName /* = wxT("svg") */)
    : wxSVGElement(tagName), m_pixelUnitToMillimeterX(0), m_pixelUnitToMillimeterY(0)
    , m_screenPixelToMillimeterX(0), m_screenPixelToMillimeterY(0), m_useCurrentView(0), m_currentScale(0)
{
    m_this = std::make_shared<wxSVGSVGElementPrivate>();
}

unsigned long wxSVGSVGElement::SuspendRedraw(unsigned long max_wait_milliseconds) {
	return 0;
}

void wxSVGSVGElement::UnsuspendRedraw(unsigned long suspend_handle_id) {

}

void wxSVGSVGElement::UnsuspendRedrawAll() {

}

void wxSVGSVGElement::ForceRedraw() {

}

void wxSVGSVGElement::PauseAnimations() {

}

void wxSVGSVGElement::UnpauseAnimations() {

}

bool wxSVGSVGElement::AnimationsPaused() {
	return false;
}

double wxSVGSVGElement::GetCurrentTime() {
	return 0;
}

void wxSVGSVGElement::SetCurrentTime(double seconds) {

}

void RecurseIntersectionList(const wxSVGSVGElement& root, const wxSVGElement& element,
		const wxSVGRect& rect, wxNodeList& res) {
	if (((wxSVGSVGElement*) &root)->CheckIntersection(element, rect)) {
		res.Add((wxSVGElement*) &element);
		wxSVGElement* n = (wxSVGElement*) element.GetChildren();
		while (n) {
			if (n->GetType() == wxSVGXML_ELEMENT_NODE)
				RecurseIntersectionList(root, *n, rect, res);
			n = (wxSVGElement*) n->GetNext();
		}
	}
}

wxNodeList wxSVGSVGElement::GetIntersectionList(const wxSVGRect& rect, const wxSVGElement& referenceElement) {
	wxNodeList res;
	RecurseIntersectionList(*this, referenceElement, rect, res);
	return res;
}

wxNodeList wxSVGSVGElement::GetEnclosureList(const wxSVGRect& rect, const wxSVGElement& referenceElement) {
	wxNodeList res;
	return res;
}

bool wxSVGSVGElement::CheckIntersection(const wxSVGElement& element, const wxSVGRect& rect) {
	wxSVGRect elemBBox = GetElementResultBBox(&element, wxSVG_COORDINATES_VIEWPORT);
	return elemBBox.GetX() + elemBBox.GetWidth() > rect.GetX()
			&& elemBBox.GetX() < rect.GetX() + rect.GetWidth()
			&& elemBBox.GetY() + elemBBox.GetHeight() > rect.GetY()
			&& elemBBox.GetY() < rect.GetY() + rect.GetHeight();
}

bool wxSVGSVGElement::CheckEnclosure(const wxSVGElement& element, const wxSVGRect& rect) {
	bool res = 0;
	return res;
}

void wxSVGSVGElement::DeselectAll() {

}

wxSVGNumber wxSVGSVGElement::CreateSVGNumber() const {
	return wxSVGNumber();
}

wxSVGLength wxSVGSVGElement::CreateSVGLength() const {
	return wxSVGLength();
}

wxSVGAngle wxSVGSVGElement::CreateSVGAngle() const {
	return wxSVGAngle();
}

wxSVGPoint wxSVGSVGElement::CreateSVGPoint() const {
	return wxSVGPoint();
}

wxSVGMatrix wxSVGSVGElement::CreateSVGMatrix() const {
	return wxSVGMatrix();
}

wxSVGRect wxSVGSVGElement::CreateSVGRect() const {
	return wxSVGRect();
}

wxSVGTransform wxSVGSVGElement::CreateSVGTransform() const {
	return wxSVGTransform();
}

wxSVGTransform wxSVGSVGElement::CreateSVGTransformFromMatrix(const wxSVGMatrix& matrix) const {
	return wxSVGTransform(matrix);
}

wxSVGElement*
wxSVGSVGElement::wxSVGSVGElementPrivate::RecurseElementId(wxSVGElement* root, const wxString& elementId)
{
	if (root->GetId() == elementId)
    {
        CacheElement(elementId, root);
		return root;
    }

	// check childs
	wxSVGElement* child = (wxSVGElement*) root->GetChildren();
	while (child) {
		if (child->GetType() == wxSVGXML_ELEMENT_NODE) {
			if (child->GetDtd() == wxSVG_SVG_ELEMENT) {
				if (child->GetId() == elementId) {
                    CacheElement(elementId, child);
					return child;
                }
			} else {
				wxSVGElement* res = RecurseElementId(child, elementId);
				if (res)
					return res;
			}
		}
		child = (wxSVGElement*) child->GetNext();
	}
	return NULL;
}

void wxSVGSVGElement::wxSVGSVGElementPrivate::CacheElementsIds(wxSVGElement* root)
{
    wxString elementId = root->GetId();
    if (!elementId.empty())
        CacheElement(elementId, root);

    for (auto* child = (wxSVGElement*) root->GetChildren();
         child;
         child = (wxSVGElement*) child->GetNext())
    {
        if (child->GetType() != wxSVGXML_ELEMENT_NODE)
            continue;
        else if (child->GetDtd() == wxSVG_SVG_ELEMENT)
        {
            elementId = child->GetId();
            if (!elementId.empty())
                CacheElement(elementId, child);
        }
        else
            CacheElementsIds(child);
    }
}

void wxSVGSVGElement::wxSVGSVGElementPrivate::CacheElement(const wxString& elementId, wxSVGElement* element)
{
    getElementByIdCache[elementId] = element;
}

wxSVGElement*
wxSVGSVGElement::wxSVGSVGElementPrivate::GetElementByIdCached(const wxString& elementId, wxSVGElement* svg)
{
    if (!elementByIdWasCached)
    {
        CacheElementsIds(svg);
        elementByIdWasCached = true;
    }

    auto it = getElementByIdCache.find(elementId);
    if (it != getElementByIdCache.end())
        return it->second;

    return RecurseElementId(svg, elementId);
}

wxSvgXmlElement* wxSVGSVGElement::GetElementById(const wxString& elementId) const
{
    auto* element = (wxSVGElement*) m_this->GetElementByIdCached(elementId, (wxSVGElement*)this);
    if (element)
        return element;

    auto* svg = GetOwnerSVGElement();
    for (; svg; svg = svg->GetOwnerSVGElement());

    return svg ? svg->GetElementById(elementId) : nullptr;
}

void wxSVGSVGElement::ParseStyleElementContent(const wxSVGStyleElement& styleElem)
{
    static const wxStringCharType* regexStr = wxT(R"((([\.|#]?[[:alpha:]](\w|-|_)*\s*,?\s*)+)\s*\{\s*([^\{\}]*)\}\s*)");
    static const std::basic_regex<wxStringCharType> regexExpr(regexStr);

    typedef std::basic_string<wxStringCharType> StringType;
    typedef std::regex_iterator<StringType::const_iterator> StringRegexIt;

    for (wxSvgXmlNode* child = styleElem.GetChildren(); child; child = child->GetNext())
    {
        if (child->GetType() != wxSVGXML_TEXT_NODE)
            continue;

        try {

        StringType content = (const wxStringCharType*)child->GetContent().data() ? : wxT("");
        StringRegexIt regexEnd, regexIt(content.cbegin(), content.cend(), regexExpr);

        for (; regexIt != regexEnd; )
        {
            if (regexIt->size() != 5)
            {
                try { ++regexIt; }
                catch(...) { break; }
                continue;
            }

            StringType classNamesStr = (*regexIt)[1].str();
            StringType cssStyleStr   = (*regexIt)[4].str();
            wxArrayString classNames = wxStringTokenize(classNamesStr, wxT(","));
            for (wxString className : classNames)
            {
                wxString classNameNoWs = className.Trim(false).Trim(true);
                const wxString classNamePrefix = classNameNoWs.substr(0, 1);

                CssStyleMap* collection = &m_this->cssElements;
                if (classNamePrefix == '.')
                {
                    classNameNoWs = classNameNoWs.substr(1);
                    collection = &m_this->cssClasses;
                }
                else if (classNamePrefix == '#')
                {
                    classNameNoWs = classNameNoWs.substr(1);
                    collection = &m_this->cssIds;
                }

                auto it = collection->find(classNameNoWs);
                if (it == collection->end())
                {
                    auto cssStyle = std::make_shared<wxCSSStyleDeclaration>();
                    cssStyle->SetCSSText(cssStyleStr);
                    collection->insert({classNameNoWs, cssStyle});
                }
                else
                {
                    wxCSSStyleDeclaration cssStyle;
                    cssStyle.SetCSSText(cssStyleStr);
                    it->second->Add(cssStyle);
                }
            }

            try { ++regexIt; }
            catch(...) { break; }
        }

        } catch(...){}
    }
}

wxCSSStyleDeclarationCPtr
wxSVGSVGElement::GetCssStyleByClass(const wxString& className)
{
    const auto it = m_this->cssClasses.find(className);
    if (it != m_this->cssClasses.end())
        return it->second;
    else
        return {};
}

wxCSSStyleDeclarationCPtr
wxSVGSVGElement::GetCssStyleById(const wxString& id)
{
    const auto it = m_this->cssIds.find(id);
    if (it != m_this->cssIds.end())
        return it->second;
    else
        return {};
}

wxCSSStyleDeclarationCPtr
wxSVGSVGElement::GetCssStyleByType(const wxString& type)
{
    const auto it = m_this->cssElements.find(type);
    if (it != m_this->cssElements.end())
        return it->second;
    else
        return {};
}
