#include "env/JaasCHTable.hpp"
#include "env/CHTable.hpp"
#include "env/PersistentCHTable.hpp"
#include "env/j9methodServer.hpp"
#include "infra/List.hpp"                      // for TR::list
#include "compile/VirtualGuard.hpp"            // for TR_VirtualGuard
#include "il/SymbolReference.hpp"              // for SymbolReference
#include "codegen/CodeGenerator.hpp"           // for CodeGenerator
#include "env/VMAccessCriticalSection.hpp"     // for VMAccessCriticalSection

void
jaasCommitVirtualGuard(const VirtualGuardInfoForCHTable *info, std::vector<TR_VirtualGuardSite> &sites,
                               TR_PersistentCHTable *table, TR::Compilation *comp);

void
jaasAddAnAssumptionForEachSubClass(TR_PersistentCHTable   *table,
                                                TR_PersistentClassInfo *clazz,
                                                std::vector<TR_VirtualGuardSite> &list,
                                                TR::Compilation *comp)
   {
   // Gather the subtree of classes rooted at clazz
   TR_ScratchList<TR_PersistentClassInfo> subTree(comp->trMemory());
   TR_ClassQueries::collectAllSubClasses(clazz, &subTree, comp);

   // Add the root to the list
   subTree.add(clazz);

   for (TR_VirtualGuardSite &site : list)
      {
      ListIterator<TR_PersistentClassInfo> classIt(&subTree);
      for (TR_PersistentClassInfo *sc = classIt.getFirst(); sc; sc = classIt.getNext())
         {
         TR_ASSERT(sc == table->findClassInfo(sc->getClassId()), "Class ID mismatch");
         TR_PatchNOPedGuardSiteOnClassExtend::make(comp->fe(), comp->trPersistentMemory(), sc->getClassId(),
                                                                  site.getLocation(),
                                                                  site.getDestination(),
                                                                  comp->getMetadataAssumptionList());
         comp->setHasClassExtendAssumptions();
         }
      }
   }

VirtualGuardInfoForCHTable getImportantVGuardInfo(TR::Compilation *comp, TR_VirtualGuard *vguard)
   {
   VirtualGuardInfoForCHTable info;
   info._testType = vguard->getTestType();
   info._kind = vguard->getKind();
   info._calleeIndex = vguard->getCalleeIndex();
   info._byteCodeIndex = vguard->getByteCodeIndex();

   TR::SymbolReference *symRef = vguard->getSymbolReference();
   TR::MethodSymbol *methodSymbol = symRef->getSymbol()->castToMethodSymbol();
   TR::ResolvedMethodSymbol *resolvedMethodSymbol = methodSymbol->getResolvedMethodSymbol();
   info._hasResolvedMethodSymbol = resolvedMethodSymbol != nullptr;
   info._cpIndex = symRef->getCPIndex();
   info._owningMethod = static_cast<TR_ResolvedJ9JAASServerMethod*>(symRef->getOwningMethod(comp))->getRemoteMirror();
   info._isInterface = methodSymbol->isInterface();
   info._guardedMethod = resolvedMethodSymbol ? static_cast<TR_ResolvedJ9JAASServerMethod*>(resolvedMethodSymbol->getResolvedMethod())->getRemoteMirror() : nullptr;
   info._offset = symRef->getOffset();

   info._isInlineGuard = vguard->isInlineGuard();
   TR_DevirtualizedCallInfo *dc;
   info._guardedMethodThisClass = vguard->isInlineGuard()
      ? vguard->getThisClass()
      : ((dc = comp->findDevirtualizedCall(vguard->getCallNode()))
         ? dc->_thisType
         : info._guardedMethod->classOfMethod());

   info._thisClass = vguard->getThisClass();
   info._mergedWithHCRGuard = vguard->mergedWithHCRGuard();
   info._mergedWithOSRGuard = vguard->mergedWithOSRGuard();

   info._mutableCallSiteObject = info._kind == TR_MutableCallSiteTargetGuard ? vguard->mutableCallSiteObject() : nullptr;
   info._mutableCallSiteEpoch = info._kind == TR_MutableCallSiteTargetGuard ? vguard->mutableCallSiteEpoch() : -1;

   return info;
   }

bool TR_CHTable::commitRemote(TR::Compilation *comp)
   {
   // collect info from TR_CHTable
   std::vector<TR_OpaqueClassBlock*> classes = _classes
      ? std::vector<TR_OpaqueClassBlock*>(&(_classes->element(0)), (&(_classes->element(_classes->lastIndex()))) + 1)
      : std::vector<TR_OpaqueClassBlock*>();
   std::vector<TR_OpaqueClassBlock*> classesThatShouldNotBeNewlyExtended = _classesThatShouldNotBeNewlyExtended
      ? std::vector<TR_OpaqueClassBlock*>(&_classesThatShouldNotBeNewlyExtended->element(0), &_classesThatShouldNotBeNewlyExtended->element(_classesThatShouldNotBeNewlyExtended->lastIndex()) + 1)
      : std::vector<TR_OpaqueClassBlock*>();
   std::vector<TR_ResolvedMethod*> preXMethods(_preXMethods ? _preXMethods->size() : 0);
   for (size_t i = 0; i < preXMethods.size(); i++)
      {
      TR_ResolvedMethod *method = _preXMethods->element(i);
      TR_ResolvedJ9JAASServerMethod *jaasMethod = static_cast<TR_ResolvedJ9JAASServerMethod*>(method);
      preXMethods[i] = jaasMethod->getRemoteMirror();
      }

   // collect virtual guard info
   TR::list<TR_VirtualGuard*> &vguards = comp->getVirtualGuards();
   std::vector<VirtualGuardForCHTable> serialVGuards;
   serialVGuards.reserve(vguards.size());

   for (TR_VirtualGuard* vguard : vguards)
      {
      VirtualGuardInfoForCHTable info = getImportantVGuardInfo(comp, vguard);

      std::vector<TR_VirtualGuardSite> nopSites;

         {
         List<TR_VirtualGuardSite> &sites = vguard->getNOPSites();
         ListIterator<TR_VirtualGuardSite> it(&sites);
         for (TR_VirtualGuardSite *site = it.getFirst(); site; site = it.getNext())
            nopSites.push_back(*site);
         }

      std::vector<VirtualGuardInfoForCHTable> innerAssumptions;
         {
         ListIterator<TR_InnerAssumption> it(&vguard->getInnerAssumptions());
         for (TR_InnerAssumption *inner = it.getFirst(); inner; inner = it.getNext())
            {
            VirtualGuardInfoForCHTable innerInfo = getImportantVGuardInfo(comp, vguard);
            innerAssumptions.push_back(innerInfo);
            }
         }

      serialVGuards.push_back(std::make_tuple(info, nopSites, innerAssumptions));
      }

   TR::list<TR_VirtualGuardSite*> &sideEffectPatchSites = *comp->getSideEffectGuardPatchSites();
   for (TR_VirtualGuardSite *site : sideEffectPatchSites)
      {
      TR_ASSERT(false, "sideEffectPathSites are unimplemented for JITaaS");
      fprintf(stderr, "code cache %p\n", comp->cg()->getBinaryBufferStart());
      fprintf(stderr, "site %p %p %p\n", site->getLocation(), site->getLocation2(), site->getDestination());
      }

   auto stream = TR::CompilationInfo::getStream();
   stream->write(JAAS::J9ServerMessageType::CHTable_commit, classes, classesThatShouldNotBeNewlyExtended, preXMethods, serialVGuards);
   auto recv = stream->read<bool>();

   return std::get<0>(recv);
   }


bool jaasCHTableCommit(
      TR::Compilation *comp,
      std::vector<TR_OpaqueClassBlock*> classes,
      std::vector<TR_OpaqueClassBlock*> classesThatShouldNotBeNewlyExtended,
      std::vector<TR_ResolvedMethod*> preXMethods,
      std::vector<TR_VirtualGuardSite> sideEffectPatchSites,
      std::vector<VirtualGuardForCHTable> vguards)
   {
   if (comp->fej9()->isAOT_DEPRECATED_DO_NOT_USE())
      return true;
   if (vguards.empty() && sideEffectPatchSites.empty() && preXMethods.empty() && classes.empty() && classesThatShouldNotBeNewlyExtended.empty())
      return true;

   //cleanupNewlyExtendedInfo(comp); TODO
   if (comp->getFailCHTableCommit())
      return false;

   TR_PersistentCHTable *table       = comp->getPersistentInfo()->getPersistentCHTable();
   TR_ResolvedMethod  *currentMethod = comp->getCurrentMethod();
   uint8_t   *startPC                = comp->cg()->getCodeStart(); //TODO is this OK?
   TR_Hotness hotness                = comp->getMethodHotness();

   if (!preXMethods.empty())
      {
      for (TR_ResolvedMethod *method : preXMethods)
         if (method->virtualMethodIsOverridden())
            return false;

      for (TR_ResolvedMethod *resolvedMethod : preXMethods)
         {
         TR_OpaqueMethodBlock *method = resolvedMethod->getPersistentIdentifier();
         TR_PreXRecompileOnMethodOverride::make(comp->fe(), comp->trPersistentMemory(), method, startPC, comp->getMetadataAssumptionList());
         comp->setHasMethodOverrideAssumptions();
         }
      }

   if (!classes.empty())
      {
      for (size_t i = 0; i < classes.size(); ++i)
         {
         TR_OpaqueClassBlock * classId = classes[i];

         // check if we may have already added this class to the persistent table
         //
         bool alreadyAdded = false;
         for (size_t j = 0; j < i && !alreadyAdded; ++j)
            {
            TR_OpaqueClassBlock * other = classes[j];
            if (other == classId)
               alreadyAdded = true;
            }

         if (!alreadyAdded)
            {
            if (comp->fe()->classHasBeenExtended(classId))
               return false;

            TR_PreXRecompileOnClassExtend::make(comp->fe(), comp->trPersistentMemory(), classId, startPC, comp->getMetadataAssumptionList());
            comp->setHasClassExtendAssumptions();
            }
         }
      }

    if (!classesThatShouldNotBeNewlyExtended.empty())
      {
      // keep a list of classes that were set visited so that we can
      // easily reset the visited flag later-on
      TR_CHTable::VisitTracker tracker(comp->trMemory());
      for (size_t i = 0; i < classesThatShouldNotBeNewlyExtended.size(); ++i)
         {
         TR_OpaqueClassBlock * classId = classesThatShouldNotBeNewlyExtended[i];
         TR_PersistentClassInfo * cl = table->findClassInfo(classId);
         if (cl && !cl->hasBeenVisited()) // is it possible to have cl==0
            {
            tracker.visit(cl);
            }
         }

      bool invalidAssumption = false;
      auto it = tracker.iterator();
      for (auto cl = it.getFirst(); cl; cl = it.getNext())
         {
         // If the class has been extended verify that was included in the original list
         // Otherwise it means that the extension happened afterwards and we should fail this compilation
         if (comp->fe()->classHasBeenExtended(cl->getClassId()))
            {
            for (TR_SubClass *subClass = cl->getFirstSubclass(); subClass; subClass = subClass->getNext())
               {
               TR_PersistentClassInfo *subClassInfo = subClass->getClassInfo();
               if (!subClassInfo->hasBeenVisited())
                  {
                  invalidAssumption = true;
                  break;
                  }
               }
            }

         if (invalidAssumption)
            break;

         TR_PreXRecompileOnClassExtend::make(comp->fe(), comp->trPersistentMemory(), cl->getClassId(), startPC, comp->getMetadataAssumptionList());
         comp->setHasClassExtendAssumptions();
         }

      if (invalidAssumption) return false;
      } //  if (classesThatShouldNotBeNewlyExtended)

   if (!vguards.empty())
      {
      static bool dontGroupOSRAssumptions = (feGetEnv("TR_DontGroupOSRAssumptions") != NULL);
      //if (!dontGroupOSRAssumptions)
         //jaasCommitOSRVirtualGuards(comp, vguards);

      for (auto &guard : vguards)
         {
         auto &info = std::get<0>(guard);
         auto &sites = std::get<1>(guard);
         auto &innerAssumptions = std::get<2>(guard);
         if (sites.empty())
            continue;

         // Commit the virtual guard itself
         //
         jaasCommitVirtualGuard(&info, sites, table, comp);

         // Commit any inner guards that are assuming on this guard
         //
         for (auto &inner : innerAssumptions)
            jaasCommitVirtualGuard(&inner, sites, table, comp);
         }
      }

   //if (!sideEffectPatchSites.empty())
      //table->commitSideEffectGuards(comp);
   return true;
   }
/* TODO
void
TR_CHTable::commitOSRVirtualGuards(TR::Compilation *comp, TR::list<TR_VirtualGuard*> &vguards)
   {
   // Count patch sites with OSR assumptions
   int osrSites = 0;
   TR_VirtualGuardSite *onlySite = NULL;
   for (auto info = vguards.begin(); info != vguards.end(); ++info)
      {
      if ((*info)->getKind() == TR_OSRGuard || (*info)->mergedWithOSRGuard())
         {
         List<TR_VirtualGuardSite> &sites = (*info)->getNOPSites();
         if (sites.getSize() > 0)
            onlySite = sites.getListHead()->getData();
         osrSites += sites.getSize();
         }
      }

   TR_Array<TR_OpaqueClassBlock*> *clazzes = comp->getClassesForOSRRedefinition();
   if (osrSites == 0 || clazzes->size() == 0)
      return;
   else if (osrSites == 1)
      {
      // Only one patch point, create an assumption for each class
      for (int i = 0; i < clazzes->size(); ++i)
         TR_PatchNOPedGuardSiteOnClassRedefinition
            ::make(comp->fe(), comp->trPersistentMemory(), (*clazzes)[i], onlySite.getLocation(), onlySite.getDestination(), comp->getMetadataAssumptionList());
      }
   else if (osrSites > 1)
      {
      // Several points to patch, create collection
      TR::PatchSites *points = new (comp->trPersistentMemory()) TR::PatchSites(comp->trPersistentMemory(), osrSites);
      for (auto info = vguards.begin(); info != vguards.end(); ++info)
         {
         if ((*info)->getKind() == TR_OSRGuard || (*info)->mergedWithOSRGuard())
            { 
            List<TR_VirtualGuardSite> &sites = (*info)->getNOPSites();
            ListIterator<TR_VirtualGuardSite> it(&sites);
            for (TR_VirtualGuardSite *site = it.getFirst(); site; site = it.getNext())
               points->add(site.getLocation(), site.getDestination());
            }
         }
 
      for (int i = 0; i < clazzes->size(); ++i)
         TR_PatchMultipleNOPedGuardSitesOnClassRedefinition
            ::make(comp->fe(), comp->trPersistentMemory(), (*clazzes)[i], points, comp->getMetadataAssumptionList());
      }

   comp->setHasClassRedefinitionAssumptions();
   return;
   }
*/
void
jaasCommitVirtualGuard(const VirtualGuardInfoForCHTable *info, std::vector<TR_VirtualGuardSite> &sites,
                               TR_PersistentCHTable *table, TR::Compilation *comp)
   {
   // If this is an OSR guard or another kind that has been marked as necessary to patch
   // in OSR, add a runtime assumption for every class that generated fear
   //
   // TODO
   /*
   static bool dontGroupOSRAssumptions = (feGetEnv("TR_DontGroupOSRAssumptions") != NULL);
   if (dontGroupOSRAssumptions && (info->getKind() == TR_OSRGuard || info->mergedWithOSRGuard()))
      {
      TR_Array<TR_OpaqueClassBlock*> *clazzes = comp->getClassesForOSRRedefinition();
      if (clazzes)
         {
         ListIterator<TR_VirtualGuardSite> it(&sites);
         for (TR_VirtualGuardSite *site = it.getFirst(); site; site = it.getNext())
            {
            for (uint32_t i = 0; i < clazzes->size(); ++i)
               TR_PatchNOPedGuardSiteOnClassRedefinition
                  ::make(comp->fe(), comp->trPersistentMemory(), (*clazzes)[i], site.getLocation(), site.getDestination(), comp->getMetadataAssumptionList());

            if (clazzes->size() > 0)
               comp->setHasClassRedefinitionAssumptions();
            }
         }

      // If this was an OSR guard, there is nothing left to do
      if (info->getKind() == TR_OSRGuard)
         return;
      }

   // OSR guards are specially handled
   if (!dontGroupOSRAssumptions && info->getKind() == TR_OSRGuard)
      return;
      */

   TR_ResolvedMethod     *guardedMethod        = 0;
   TR_OpaqueClassBlock     *guardedClass         = 0;
   bool                     nopAssumptionIsValid = true;
   int32_t                  cpIndex              = info->_cpIndex;
   TR_ResolvedMethod     *owningMethod         = info->_owningMethod;

   if ((info->_kind == TR_HCRGuard) || info->_mergedWithHCRGuard)
      {
      guardedClass = info->_thisClass;
      for (TR_VirtualGuardSite &site : sites)
         {
         TR_ASSERT(site.getLocation(), "assertion failure");
         TR_PatchNOPedGuardSiteOnClassRedefinition
            ::make(comp->fe(), comp->trPersistentMemory(), guardedClass, site.getLocation(), site.getDestination(), comp->getMetadataAssumptionList());
         comp->setHasClassRedefinitionAssumptions();
         }
      // if it's not real HCR guard then we need to register
      // both the HCR site and the guard
      if (!info->_mergedWithHCRGuard)
         return;
      if (!info->_kind != TR_ProfiledGuard) // isNopable
         return;
      }

   if (info->_kind == TR_DummyGuard)
      {
      // nothing to do
      }
   else if (info->_kind == TR_MutableCallSiteTargetGuard)
      {
      static char *dontInvalidateMCSTargetGuards = feGetEnv("TR_dontInvalidateMCSTargetGuards");
      if (!dontInvalidateMCSTargetGuards)
         {
         uintptrj_t *mcsReferenceLocation = info->_mutableCallSiteObject;
         TR::KnownObjectTable *knot = comp->getKnownObjectTable();
         TR_ASSERT(knot, "MutableCallSiteTargetGuard requires the Known Object Table");
         void *cookiePointer = comp->trPersistentMemory()->allocatePersistentMemory(1);
         uintptrj_t potentialCookie = (uintptrj_t)(uintptr_t)cookiePointer;
         uintptrj_t cookie = 0;

         TR::KnownObjectTable::Index currentIndex;

            {
            TR_J9VMBase *fej9 = (TR_J9VMBase *)(comp->fe());
            TR::VMAccessCriticalSection invalidateMCSTargetGuards(fej9);
            // TODO: Code duplication with TR_InlinerBase::findInlineTargets
            currentIndex = TR::KnownObjectTable::UNKNOWN;
            uintptrj_t currentEpoch = fej9->getVolatileReferenceField(*mcsReferenceLocation, "epoch", "Ljava/lang/invoke/MethodHandle;");
            if (currentEpoch)
               currentIndex = knot->getIndex(currentEpoch);
            if (info->_mutableCallSiteEpoch == currentIndex)
               cookie = fej9->mutableCallSiteCookie(*mcsReferenceLocation, potentialCookie);
            else
               nopAssumptionIsValid = false;
            }

         if (cookie != potentialCookie)
            comp->trPersistentMemory()->freePersistentMemory(cookiePointer);

         if (nopAssumptionIsValid)
            {
            for (TR_VirtualGuardSite &site : sites)
               {
               TR_ASSERT(site.getLocation(), "assertion failure");
               TR_PatchNOPedGuardSiteOnMutableCallSiteChange
                  ::make(comp->fe(), comp->trPersistentMemory(), cookie, site.getLocation(), site.getDestination(), comp->getMetadataAssumptionList());
               }
            }
         else if (comp->getOption(TR_TraceCG))
            traceMsg(comp, "MutableCallSiteTargetGuard is already invalid.  Expected epoch: obj%d  Found: obj%d\n", info->_mutableCallSiteEpoch, currentIndex);
         }
      }
   else if ((info->_kind == TR_MethodEnterExitGuard) || (info->_kind == TR_DirectMethodGuard))
      {
      // nothing to do
      }
   else if (info->_kind == TR_BreakpointGuard)
      {
      if (comp->getOption(TR_DisableNopBreakpointGuard))
         return;
      TR_ResolvedMethod *breakpointedMethod = comp->getInlinedResolvedMethod(info->_calleeIndex);
      TR_OpaqueMethodBlock *method = breakpointedMethod->getPersistentIdentifier();
      if (comp->fej9()->isMethodBreakpointed(method))
         nopAssumptionIsValid = false;
      for (TR_VirtualGuardSite &site : sites)
         {
         TR_PatchNOPedGuardSiteOnMethodBreakPoint
            ::make(comp->fe(), comp->trPersistentMemory(), method, site.getLocation(), site.getDestination(), comp->getMetadataAssumptionList());
         }
      }
   else if (info->_kind == TR_ArrayStoreCheckGuard)
      {
      guardedClass = info->_thisClass;
      if (comp->fe()->classHasBeenExtended(guardedClass))
         nopAssumptionIsValid = false;
      }
   else if (!info->_hasResolvedMethodSymbol)
      {
      TR_ASSERT(info->_isInterface && info->_kind == TR_InterfaceGuard, "assertion failure");
      TR_OpaqueClassBlock *thisClass = info->_thisClass;
      TR_ASSERT(thisClass, "assertion failure");
      
      TR_ResolvedMethod *implementer = table->findSingleImplementer(thisClass, cpIndex, owningMethod, comp, true, TR_yes);
      if (!implementer ||
          (info->_testType == TR_VftTest && comp->fe()->classHasBeenExtended(implementer->containingClass())))
         nopAssumptionIsValid = false;
      else
         jaasAddAnAssumptionForEachSubClass(table, table->findClassInfo(thisClass), sites, comp);
      }
   else if (info->_kind == TR_NonoverriddenGuard && info->_testType != TR_VftTest)
      {
      TR_ASSERT(info->_testType == TR_NonoverriddenTest, "unexpected test type for a non-overridden guard.");

      guardedMethod = info->_guardedMethod;
      if (guardedMethod->virtualMethodIsOverridden())
         nopAssumptionIsValid = false;
      }
   else if ((!info->_isInlineGuard &&
          !TR::Compiler->cls.isAbstractClass(comp, info->_guardedMethod->containingClass())) ||
          (info->_isInlineGuard && info->_kind == TR_HierarchyGuard && info->_testType == TR_MethodTest))
      {
      guardedMethod = info->_guardedMethod;
      TR_ASSERT(guardedMethod, "Method must have been found when creating assumption\n");

      TR_OpaqueClassBlock *thisClass = info->_guardedMethodThisClass;

     //temp fix for defect 57599 until we find root cause
      if (table->isOverriddenInThisHierarchy(guardedMethod, thisClass, info->_offset, comp))
         nopAssumptionIsValid = false;
      }
   else if (info->_isInlineGuard &&
            info->_testType == TR_VftTest &&
            (info->_kind == TR_NonoverriddenGuard || info->_kind == TR_HierarchyGuard))
      {
      guardedClass = info->_thisClass;
      if (comp->fe()->classHasBeenExtended(guardedClass))
         nopAssumptionIsValid = false;
      }
   else if ((!info->_isInlineGuard &&
             TR::Compiler->cls.isAbstractClass(comp, info->_guardedMethod->containingClass())) ||
            (info->_isInlineGuard && info->_kind == TR_AbstractGuard && info->_testType == TR_MethodTest))
      {
      TR_OpaqueClassBlock *thisClass = info->_isInlineGuard ? thisClass = info->_thisClass
         : info->_guardedMethod->containingClass();
      if (table->findSingleAbstractImplementer(thisClass, info->_offset, owningMethod, comp, true))
         jaasAddAnAssumptionForEachSubClass(table, table->findClassInfo(thisClass), sites, comp);
      else
         nopAssumptionIsValid = false;
      }
   else
      {
      TR_ASSERT(false, "should be unreachable");
      nopAssumptionIsValid = false;
      }

   if (nopAssumptionIsValid)
      {
      for (TR_VirtualGuardSite &site : sites)
         {
         TR_ASSERT(site.getLocation(), "assertion failure");
         if (guardedClass)
            {
            TR_PatchNOPedGuardSiteOnClassExtend
               ::make(comp->fe(), comp->trPersistentMemory(), guardedClass, site.getLocation(), site.getDestination(), comp->getMetadataAssumptionList());
            comp->setHasClassExtendAssumptions();
            }
         if (guardedMethod)
            {
            TR_PatchNOPedGuardSiteOnMethodOverride
               ::make(comp->fe(), comp->trPersistentMemory(), guardedMethod->getPersistentIdentifier(),
                site.getLocation(), site.getDestination(), comp->getMetadataAssumptionList());
            comp->setHasMethodOverrideAssumptions();
            }
         }
      }
   else // assumption is invalid
      {
      for (TR_VirtualGuardSite &site : sites)
         {
         if (comp->getOption(TR_TraceCG))
            traceMsg(comp, "   Patching %p to %p\n", site.getLocation(), site.getDestination());
         TR::PatchNOPedGuardSite::compensate(0, site.getLocation(), site.getDestination());
         }
      }
   }

