// Local evidence probe: no upstream types/symbols are imported.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import com.google.gson.*;
import java.nio.file.*;
import java.util.*;

public class GodHandProbe extends GhidraScript {
    private JsonObject instruction(Instruction ins) throws Exception {
        JsonObject j = new JsonObject();
        j.addProperty("address", ins.getAddress().toString());
        j.addProperty("instruction", ins.toString());
        StringBuilder hex = new StringBuilder();
        for(byte b : ins.getBytes()) hex.append(String.format("%02x", b & 255));
        j.addProperty("bytesLittleEndian", hex.toString());
        byte[] bytes=ins.getBytes();
        if(bytes.length==4) {
            long word=0;
            for(int n=0;n<4;n++)word|=(long)(bytes[n]&255)<<(8*n);
            j.addProperty("wordHex",String.format("0x%08x",word));
            // Record canonical transfer fields from the opcode, independently of Sleigh display aliases.
            if((word>>>26)==0x11 && ((word>>>21)&31)==6) {
                j.addProperty("canonicalCop1Transfer", "ctc1 GPR["+((word>>>16)&31)+"], FCR["+((word>>>11)&31)+"]");
            }
        }
        j.addProperty("flowType", ins.getFlowType().toString());
        JsonArray targets = new JsonArray();
        for(Address a : ins.getFlows()) targets.add(a.toString());
        j.add("flowTargets", targets);
        return j;
    }
    private JsonArray references(Address a) {
        JsonArray refs = new JsonArray();
        for(Reference r : currentProgram.getReferenceManager().getReferencesTo(a)) {
            JsonObject j = new JsonObject();
            j.addProperty("from", r.getFromAddress().toString());
            j.addProperty("type", r.getReferenceType().toString());
            j.addProperty("source", r.getSource().toString());
            refs.add(j);
        }
        return refs;
    }
    public void run() throws Exception {
        String[] args = getScriptArgs();
        JsonObject out = new JsonObject();
        out.addProperty("program", currentProgram.getName());
        out.addProperty("executablePath", currentProgram.getExecutablePath());
        out.addProperty("language", currentProgram.getLanguageID().toString());
        out.addProperty("compiler", currentProgram.getCompilerSpec().getCompilerSpecID().toString());
        out.addProperty("imageBase", currentProgram.getImageBase().toString());
        out.addProperty("analysisPolicy", "headless -noanalysis; explicit local disassembly/function creation only; no upstream symbols or types");
        out.addProperty("analyzedFlag", currentProgram.getOptions(Program.PROGRAM_INFO).getBoolean("Analyzed", false));
        out.addProperty("xrefLimitation", "References include only imported metadata and explicitly disassembled control flow; whole-image xrefs are incomplete.");
        byte[] elf = Files.readAllBytes(Paths.get(currentProgram.getExecutablePath().replaceFirst("^/([A-Za-z]:)", "$1")));
        StringBuilder digest=new StringBuilder();
        for(byte b:java.security.MessageDigest.getInstance("SHA-256").digest(elf))digest.append(String.format("%02x",b&255));
        out.addProperty("elfSha256",digest.toString());
        out.addProperty("elfSize",elf.length);
        long flags = 0;
        for(int n=0;n<4;n++) flags |= (long)(elf[36+n]&255) << (8*n);
        out.addProperty("elfFlagsHex", String.format("0x%08x", flags));
        out.addProperty("elfMachine", (elf[18]&255) | ((elf[19]&255)<<8));
        out.addProperty("processorSelection", "Explicit PS2 R5900 extension selected; ELF machine and flags recorded, not treated as recovered C type/prototype evidence.");
        out.addProperty("symbolPolicy", "No upstream or user symbols/types imported. FUN_/DAT_/uRam names are generated. MMIO names and instruction aliases such as clear fcsr are supplied by the installed PS2 language specification; no FPUctc1 symbol was invented.");
        JsonArray probes = new JsonArray();
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        LinkedHashMap<String,Integer> queued = new LinkedHashMap<>();
        int maxInstructions=2048;
        for(int i=1;i<args.length;i++) {
            if(args[i].equals("max-insns")) {
                if(++i==args.length) throw new IllegalArgumentException("max-insns requires a value");
                maxInstructions=Integer.parseInt(args[i]);
            }
            else if(args[i].startsWith("max-insns=")) maxInstructions=Integer.parseInt(args[i].substring("max-insns=".length()));
            // Ghidra flow targets use canonical lower-case addresses. Normalize
            // explicit requests too so auto expansion cannot duplicate the same
            // function when a caller supplied upper-case hexadecimal digits.
            else queued.put(toAddr(args[i]).toString(), 0);
        }
        if(maxInstructions<1 || maxInstructions>65536) throw new IllegalArgumentException("max-insns must be 1..65536");
        out.addProperty("rawInstructionLimit",maxInstructions);
        List<String> pending = new ArrayList<>(queued.keySet());
        for (int i = 0; i < pending.size() && i < 30; i++) {
            JsonObject p = new JsonObject();
            String address = pending.get(i);
            int depth = queued.get(address);
            p.addProperty("requestedAddress", address);
            p.addProperty("callExpansionDepth", depth);
            try {
                Address a = toAddr(address);
                p.addProperty("mapped", currentProgram.getMemory().contains(a));
                disassemble(a);
                Function f = getFunctionAt(a);
                // ELF loader creates a one-byte entry placeholder; rebuild its body from actual decoded CFG.
                if(f != null && f.getBody().getNumAddresses() <= 1) {
                    currentProgram.getFunctionManager().removeFunction(a);
                    f = null;
                }
                if (f == null) f = createFunction(a, null);
                if (f == null) f = getFunctionContaining(a);
                if (f == null) throw new IllegalStateException("No function created at requested address");
                p.addProperty("entry", f.getEntryPoint().toString());
                p.addProperty("name", f.getName());
                p.addProperty("bodyAddresses", f.getBody().getNumAddresses());
                JsonArray asm = new JsonArray();
                JsonArray raw = new JsonArray();
                JsonArray calls = new JsonArray();
                InstructionIterator it = currentProgram.getListing().getInstructions(f.getBody(), true);
                int count = 0;
                while(it.hasNext() && count < maxInstructions) {
                    Instruction ins = it.next();
                    count++;
                    asm.add(ins.getAddress().toString() + " " + ins.toString());
                    raw.add(instruction(ins));
                    if(ins.getFlowType().isCall()) {
                        calls.add(instruction(ins));
                        // Expand only the startup region, two call levels at most. RNG callees are recorded but not guessed.
                        if(a.getOffset()<0x110000 && depth<2) for(Address target : ins.getFlows()) {
                            String key=target.toString();
                            if(!queued.containsKey(key) && currentProgram.getMemory().contains(target)) {
                                queued.put(key, depth+1); pending.add(key);
                            }
                        }
                    }
                }
                p.add("disassembly", asm);
                p.add("rawInstructions", raw);
                p.add("outgoingCalls", calls);
                p.addProperty("instructionLimit",maxInstructions);
                p.addProperty("instructionCount",count);
                p.addProperty("disassemblyTruncated",it.hasNext());
                DecompileResults r = di.decompileFunction(f, 45, monitor);
                p.addProperty("decompileCompleted", r.decompileCompleted());
                p.addProperty("decompileError", r.getErrorMessage());
                if(r.getDecompiledFunction() != null) {
                    String c=r.getDecompiledFunction().getC();
                    p.addProperty("decompiledC",c);
                    JsonArray warnings=new JsonArray();
                    for(String line:c.split("\\R"))if(line.contains("WARNING:"))warnings.add(line.trim());
                    p.add("decompilerWarnings",warnings);
                }
            } catch(Exception ex) {
                p.addProperty("error", ex.toString());
            }
            probes.add(p);
        }
        // Nearby scan is explicitly speculative; no function starts or upstream names are imposed.
        Address nearStart=toAddr("0031ce50"), nearEnd=toAddr("0031d100");
        for(long addr=nearStart.getOffset();addr<nearEnd.getOffset();addr+=4) {
            Address a=toAddr(addr);
            if(getInstructionAt(a)==null) disassemble(a);
        }
        JsonObject nearby=new JsonObject();
        nearby.addProperty("range", "0031ce50..0031d100 (end exclusive)");
        nearby.addProperty("policy", "Speculative instruction scan of requested neighborhood; only explicit probes are asserted function starts.");
        JsonArray nearbyInstructions=new JsonArray();
        InstructionIterator ni=currentProgram.getListing().getInstructions(new AddressSet(nearStart, nearEnd.subtract(1)), true);
        while(ni.hasNext()) nearbyInstructions.add(instruction(ni.next()));
        nearby.add("rawInstructions", nearbyInstructions);
        JsonArray nearbyFunctions=new JsonArray();
        FunctionIterator fi=currentProgram.getFunctionManager().getFunctions(new AddressSet(nearStart, nearEnd.subtract(1)), true);
        while(fi.hasNext()) { Function f=fi.next(); JsonObject j=new JsonObject();j.addProperty("entry",f.getEntryPoint().toString());j.addProperty("name",f.getName());nearbyFunctions.add(j); }
        nearby.add("functions", nearbyFunctions);
        out.add("rngNeighborhood",nearby);
        for(JsonElement e:probes) { JsonObject p=e.getAsJsonObject();if(p.has("entry"))p.add("incomingReferences",references(toAddr(p.get("entry").getAsString()))); }
        di.dispose();
        out.add("functions", probes);
        out.addProperty("functionCountAfterProbe", currentProgram.getFunctionManager().getFunctionCount());
        Files.writeString(Paths.get(args[0]), new GsonBuilder().setPrettyPrinting().create().toJson(out));
        println("GODHAND_PROBE_OUTPUT=" + args[0]);
    }
}
