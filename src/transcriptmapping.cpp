// transcriptmapping.cpp
#include "transcriptmapping.h"

using std::string;

Gene::Gene(string id, int st, int en, int snd): Interval(st, en, snd), gene_id(id) {}
Gene::Gene(string id, int snd): Interval(-1, -1, snd), gene_id(id) {}
Gene::Gene(): Interval(-1, -1, 0), gene_id(""){}

void Gene::set_ID(string id)
{
    gene_id = id;
}

void Gene::add_exon(Interval it)
{
        exon_vec.push_back(it);
        // expand the gene interval to include the new exon
        if (st>it.st || st<0)
        {
            st = it.st;
        }
        if (en < it.en || en<0)
        {
            en = it.en;
        }
        if (snd == 0)
        {
            snd = it.snd;
        }
}

//int Gene::distance_to_end(Interval it)
//{
//    int distance = 0;
//    int tmp_en = 0;
//    auto iter = std::lower_bound(exon_vec.begin(), exon_vec.end(), it);
//    if (snd == 1)
//    {
//        distance += iter->en - ((iter->st)>it.st?(iter->st):it.st);
//        tmp_en = iter->en;
//        for (auto i = iter+1; i != exon_vec.end(); ++i)
//        {
//            if (tmp_en < i->st)
//            {
//                distance += i->en - i->st;
//                tmp_en = i->en;
//            }
//
  //      }
//
  //  }
  //  else if (snd == -1)
  //  {
  //      for (auto i = exon_vec.begin(); i != iter; ++i)
  //      {
  //          if (tmp_en < i->st)
  //          {
  //              distance += i->en - i->st;
  //              tmp_en = i->en;
  //          }
  //      }
  //      if (tmp_en < iter->st)
  //      {
//            distance += ((iter->en)<it.en?(iter->en):it.en) - iter->st;
//        }
//    }
//
//    return distance;
//}


void Gene::cal_distance_dict()
{
    int csum = 0;
    if(snd == -1)
    {
        for (auto i = exon_vec.begin(); i != exon_vec.end(); ++i)
        {
            csum_dist.push_back(csum);
            csum += i->en - i->st;
        }
    }
    else
    {
        for (auto ri = exon_vec.rbegin(); ri!= exon_vec.rend(); ++ri) // reverse
        {
            csum_dist.push_back(csum);
            csum += ri->en - ri->st;
        }
        std::reverse(csum_dist.begin(),csum_dist.end()); // reverse the dist
    }
}


int Gene::in_exon(Interval it)
{
    auto iter = std::equal_range(exon_vec.begin(), exon_vec.end(), it);
    int ret = -1; // return -1 if not in exon
    if ((iter.second-iter.first) > 0)
    {
        if (snd == -1) // reverse strand
        {
            ret = it.en<((iter.second-1)->en) ? it.en:(iter.second-1)->en;
            ret = csum_dist[iter.second-exon_vec.begin()-1]+ret-(iter.second-1)->st;
        }
        else
        {
            ret = it.st>(iter.first->st) ? it.st:(iter.first->st);
            ret = (iter.first->en) - ret + csum_dist[iter.first-exon_vec.begin()];
        }
    }
    return ret;
}

int Gene::in_exon(Interval it, bool check_strand)
{
    if (check_strand && (it.snd*snd == -1))
    {
        return -1; // if not the same strand, return -1
    }
    else
    {
        return in_exon(it);
    }
}

void Gene::sort_exon(bool flat_exon)
{
    std::sort(exon_vec.begin(), exon_vec.end());
    std::vector<Interval> new_exon;
    int new_st, new_en;
    if(flat_exon)
    {
        for (auto i = exon_vec.begin(); i != exon_vec.end(); ++i)
        {
            if (new_exon.size() == 0)
            {
                new_exon.push_back(*i);
            }
            else
            {
                if (i->st < new_exon.back().en)
                {
                    new_st = new_exon.back().st<(i->st) ? new_exon.back().st:(i->st);
                    new_en = new_exon.back().en>(i->en) ? new_exon.back().en:(i->en);
                    new_exon.pop_back();
                    new_exon.push_back(Interval(new_st, new_en, i->snd));
                }
                else
                {
                    new_exon.push_back(*i);
                }
            }
        }
        if(exon_vec.back().st > new_exon.back().en)
        {
            new_exon.push_back(exon_vec.back());
        }
        exon_vec = new_exon;
    }
}

std::ostream& operator<< (std::ostream& out, const Gene& obj)
{
    out << "Gene ID:   " << obj.gene_id  << std::endl;
    out << "\t" << "start/end:   " << obj.st  << "/" << obj.en << std::endl;
    out << "\t" << "strand:   " << obj.snd  << std::endl;
    out << "\t" << "number of exons:   " << obj.exon_vec.size()  << std::endl;
    for (int i = 0; i < obj.exon_vec.size(); ++i)
    {
        out << "\t" << "exon[" << i+1 << "]: (" << obj.exon_vec[i].st << ", " << obj.exon_vec[i].en << ")" << std::endl;
    }
    return out;
}

int GeneAnnotation::get_strand(char st)
{
    int strand = 0;
    if (st == '+')
    {
        strand = 1;
    }
    else if (st == '-')
    {
        strand = -1;
    }
    return strand;
}

string GeneAnnotation::get_parent(string tok)
{
    string parent = "";
    auto subtoken = split(tok, ';');
    for (auto attr : subtoken)
    {
        if (attr.substr(0,6) == "Parent")
        {
            parent = split(split(attr, '=')[1],':')[1];
        }
    }
    return parent;
}

string GeneAnnotation::get_ID(string tok)
{
    string ID = "";
    auto subtoken = split(tok, ';');
    for (auto attr : subtoken)
    {
        if (attr.substr(0,2) == "ID")
        {
            ID = split(split(attr, '=')[1],':')[1];
        }
    }
    return ID;
}

string GeneAnnotation::fix_name(string na)
{
    string new_na;
    if (na.compare(0,3,"chr") == 0)
    {
        return na.substr(3);
    }
    else if (na.length() > 4) // just fix 1-22, X, Y, MT. ignore contig and ERCC
    {
        return na;
    }
    else
    {
        if (na == "MT")
        {
            new_na = "chrM";
        }
        else if(na == "chrM")
        {
            new_na = "MT";
        }
        else
        {
            return na;
        }
        return new_na;
    }
}

void GeneAnnotation::parse_gff3_annotation(string gff3_fn, bool fix_chrname)
{
    std::ifstream infile(gff3_fn);

    string line;
    string ID;
    string parent;
    std::unordered_map<string, string> tmp_trans_dict; // store transcript - gene mapping
    std::unordered_map<string, std::unordered_map<string, Gene>> tmp_gene_dict;
    int strand = 0;
    std::vector<string> token;

    //std::cout << "build annotation from gff3 file..." << std::endl;
    while(std::getline(infile, line))
    {
        if (line[0] == '#')
        {
            continue; // skip header
        }

        token = split(line, '\t');
        parent = get_parent(token[8]);
        strand = get_strand(token[6][0]);
        if (token[2] == "exon")
        {
            if (tmp_trans_dict.end() != tmp_trans_dict.find(parent))
            {
                if (fix_chrname)
                {
                    tmp_gene_dict[fix_name(token[0])][tmp_trans_dict[parent]].add_exon(Interval(std::atoi(token[3].c_str()), std::atoi(token[4].c_str()), strand));
                    tmp_gene_dict[fix_name(token[0])][tmp_trans_dict[parent]].set_ID(tmp_trans_dict[parent]);
                }
                else
                {
                    tmp_gene_dict[token[0]][tmp_trans_dict[parent]].add_exon(Interval(std::atoi(token[3].c_str()), std::atoi(token[4].c_str()), strand));
                    tmp_gene_dict[token[0]][tmp_trans_dict[parent]].set_ID(tmp_trans_dict[parent]);
                }
            }
            else
            {
                std::cout << "cannot find grandparent for exon:" << std::endl;
                std::cout << line << std::endl;
                exit(EXIT_FAILURE);
            }

        }

        else if (!parent.empty())
        {
            ID = get_ID(token[8]);
            if (!ID.empty())
            {
                tmp_trans_dict[ID] = parent;
            }
        }

    }
    sort_annotation(tmp_gene_dict);
}


void GeneAnnotation::parse_bed_annotation(string bed_fn, bool fix_chrname)
{
    std::ifstream infile(bed_fn);

    string line;
    std::unordered_map<string, std::unordered_map<string, Gene>> tmp_gene_dict;
    int strand = 0;
    std::vector<string> token;

    std::getline(infile, line); // skip the header
    while(std::getline(infile, line))
    {
        token = split(line, '\t');
        strand = get_strand(token[4][0]);
        if (fix_chrname)
        {
            tmp_gene_dict[fix_name(token[1])][token[0]].add_exon(Interval(std::atoi(token[2].c_str()), std::atoi(token[3].c_str()), strand));
            tmp_gene_dict[fix_name(token[1])][token[0]].set_ID(token[0]);
        }
        else
        {
            tmp_gene_dict[token[1]][token[0]].add_exon(Interval(std::atoi(token[2].c_str()), std::atoi(token[3].c_str()), strand));
            tmp_gene_dict[token[1]][token[0]].set_ID(token[0]);
        }
    }
    sort_annotation(tmp_gene_dict);
}


void GeneAnnotation::sort_annotation(std::unordered_map<string, std::unordered_map<string, Gene>>& tmp_gene_dict)
{
    int max_bin;
    std::vector<int> tmp_idx;
    for (auto iter : tmp_gene_dict)
    {
        for (auto sub_iter : iter.second)
        {
            if (sub_iter.second.exon_vec.size()>1)
            {
                sub_iter.second.sort_exon(true);
            }
            sub_iter.second.cal_distance_dict();
            gene_dict[iter.first].push_back(sub_iter.second);
        }
        if (gene_dict[iter.first].size()>1)
        {
            std::sort(gene_dict[iter.first].begin(), gene_dict[iter.first].end(),[](Gene a, Gene b) {
                        return a.st < b.st;   
                    });
        }
        max_bin = (int) std::floor(gene_dict[iter.first].back().exon_vec.back().en/BIN_SIZE);
        for (int i=0; i<=max_bin; i++)
        {
            std::vector<int> tmp_idx;
            auto upper = std::upper_bound(gene_dict[iter.first].begin(), gene_dict[iter.first].end(), Interval(i*BIN_SIZE,(i+1)*BIN_SIZE+100));
            for (auto it=gene_dict[iter.first].begin();it<upper; it++)
            {
                if (it->overlap(i*BIN_SIZE,(i+1)*BIN_SIZE+100) == 0)
                {
                    tmp_idx.push_back(it-gene_dict[iter.first].begin());
                }
            }
            bins_dict[iter.first].push_back(tmp_idx);
        }
    }
}


int GeneAnnotation::ngenes()
{
    int gene_number = 0;
    for (auto iter : gene_dict)
    {
        for (auto sub_iter : iter.second)
        {
            gene_number ++;
        }
    }

    return gene_number;
}


std::vector<string> GeneAnnotation::get_genelist()
{
    std::vector<string> gene_list;
    for (auto iter : gene_dict)
    {
        for (auto sub_iter : iter.second)
        {
            gene_list.push_back(sub_iter.gene_id);
        }
    }

    return gene_list;
}


std::ostream& operator<< (std::ostream& out, const GeneAnnotation& obj)
{
    out << "annotation statistics:" << std::endl;
    for ( const auto& n : obj.gene_dict )
    {
        out << "\t" << "chromosome:[" << n.first << "] number of genes:[" << n.second.size() << "]\n";
    }
    for ( const auto& n : obj.gene_dict )
    {
        out << "first gene in chromosome " << n.first << " :" << std::endl;
        out << n.second[0] << std::endl;
        //break;
    }
    return out;
}


void Mapping::add_annotation(string gff3_fn, bool fix_chrname)
{
    if (gff3_fn.substr(gff3_fn.find_last_of(".") + 1) == "gff3")
    {
        std::cout << "add gff3 annotation: " << gff3_fn << std::endl;
        Anno.parse_gff3_annotation(gff3_fn, fix_chrname);
    }
    else
    {
        Anno.parse_bed_annotation(gff3_fn, fix_chrname);
        std::cout << "add bed annotation: " << gff3_fn << std::endl;
    }

}

int Mapping::map_exon(bam_hdr_t *header, bam1_t *b, string& gene_id, bool m_strand)
{
    int ret = 9999;
    int rev = bam_is_rev(b)?(-1):1;
    uint32_t* cig = bam_get_cigar(b);
    int tmp_pos = b->core.pos;
    int tmp_rest = 9999999; // distance to end pos
    int tmp_ret;
    int tmp_inexon;
    int gene_idx;
    int bin1;
    string tmp_id;
    gene_id = "";

    for (int c=0; c<b->core.n_cigar; c++)
    {
        tmp_ret = 9999;
        // *   bit 1 set if the cigar operation consumes the query
        // *   bit 2 set if the cigar operation consumes the reference
        if (((bam_cigar_type(cig[c]) >> 0) & 1) && ((bam_cigar_type(cig[c]) >> 1) & 1))
        {
            Interval it = Interval(tmp_pos, tmp_pos+bam_cigar_oplen(cig[c]), rev);
            bin1 = (int) std::floor(it.st/BIN_SIZE);

            if (Anno.bins_dict[header->target_name[b->core.tid]][bin1].size() == 0)
            {
                tmp_ret = tmp_ret>3?3:tmp_ret;
            }
            else if (Anno.bins_dict[header->target_name[b->core.tid]].size()<=bin1)
            {
                tmp_ret = tmp_ret>3?3:tmp_ret;
            }
            else
            {
                tmp_id = "";
                for (int i = 0; i < Anno.bins_dict[header->target_name[b->core.tid]][bin1].size(); ++i)
                {
                    gene_idx = Anno.bins_dict[header->target_name[b->core.tid]][bin1][i];
                    tmp_inexon = Anno.gene_dict[header->target_name[b->core.tid]][gene_idx].in_exon(it, m_strand);
                    if (tmp_inexon > -1)
                    {
                        if (tmp_id != "")
                        {
                            if (tmp_id != Anno.gene_dict[header->target_name[b->core.tid]][gene_idx].gene_id)
                            {
                                tmp_ret = 1; // ambiguous mapping
                                break;
                            }
                            else
                            {
                                // update the distance to end pos
                                tmp_rest = tmp_rest>tmp_inexon ? tmp_rest:tmp_inexon;
                            }
                        }
                        else
                        {
                            tmp_id = Anno.gene_dict[header->target_name[b->core.tid]][gene_idx].gene_id;
                            tmp_ret = 0;
                            tmp_rest = tmp_inexon;
                        }
                    }
                    else if (it > Anno.gene_dict[header->target_name[b->core.tid]][gene_idx])
                    {
                        tmp_ret = tmp_ret>=3?3:tmp_ret;
                    }
                    else if(it < Anno.gene_dict[header->target_name[b->core.tid]][gene_idx])
                    {
                        tmp_ret = tmp_ret>=3?3:tmp_ret;
                        break;
                    }
                    else
                    {
                        tmp_ret = tmp_ret>=2?2:tmp_ret;
                    }
                }
            }

            tmp_pos = tmp_pos+bam_cigar_oplen(cig[c]);
            if (ret == 0 && tmp_ret == 0)
            {
                if (gene_id != "" && gene_id != tmp_id)
                {
                    ret = 1; // still ambiguous
                    break;
                }
            }
            else if (tmp_ret == 0)
            {
                ret = 0;
                gene_id = tmp_id;
            }
            else
            {
                ret = ret<tmp_ret?ret:tmp_ret; // choose the smallest
            }
        }
        else if (!((bam_cigar_type(cig[c]) >> 0) & 1) && ((bam_cigar_type(cig[c]) >> 1) & 1))
        {
            tmp_pos = tmp_pos+bam_cigar_oplen(cig[c]);
        }
    }
    if (ret == 0)
    {
        return -tmp_rest;
    }
    else
    {
        return ret;
    }
}

void Mapping::parse_align(string fn, string fn_out, bool m_strand, string map_tag, string gene_tag, string cellular_tag, string molecular_tag, int bc_len, int UMI_len)
{
    int unaligned = 0;
    int ret;

    check_file_exists(fn); // htslib does not check if file exist so we do it manually

    // open files
    bam1_t *b = bam_init1();
    BGZF *fp = bgzf_open(fn.c_str(), "r"); // input file
    samFile *of = sam_open(fn_out.c_str(), "wb"); // output file

    bam_hdr_t *header = bam_hdr_read(fp);
    sam_hdr_write(of, header);

    string gene_id;
    int tmp_c[4] = {0,0,0,0};

    bool found_any = false;
    #pragma omp parallel for
    for (int i = 0; i < header->n_targets; ++i)
    {
        if (Anno.gene_dict.end() == Anno.gene_dict.find(header->target_name[i]))
        {
            std::cout << header->target_name[i] << " not found in exon annotation." << std::endl;
        }
        else
        {
            found_any = true;
        }

    }
    if (!found_any)
    {
        std::cout << "ERROR: The annotation and .bam file contains different chromosome." << std::endl;
        exit(1);
    }
    // for moving barcode and UMI from sequence name to bam tags
    const char * g_ptr = gene_tag.c_str();
    const char * c_ptr = cellular_tag.c_str();
    const char * m_ptr = molecular_tag.c_str();
    const char * a_ptr = map_tag.c_str();
    char buf[999] = ""; // assume the length of barcode or UMI is less than 999
    int cnt = 1;
    while (bam_read1(fp, b) >= 0)
    {
        if (__DEBUG)
        {
            if (cnt % 1000000 == 0)
            {
                std::cout << "number of read processed:" << cnt << std::endl;
                std::cout << tmp_c[0] <<"\t"<< tmp_c[1] <<"\t"<<tmp_c[2] <<"\t"<<tmp_c[3] <<"\t" << std::endl;
            }
            cnt++;
        }
        if ((b->core.flag&BAM_FUNMAP) > 0)
        {
            unaligned++;
            ret = 4;
        }
        else
        {
            //  chromosome not found in annotation:
            if (Anno.gene_dict.end() == Anno.gene_dict.find(header->target_name[b->core.tid]))
            {
                ret = 3;
            }
            else
            {
                ret = map_exon(header, b, gene_id, m_strand);
            }
            if (ret <= 0)
            {
                tmp_c[0]++;
                bam_aux_append(b, g_ptr, 'Z', gene_id.size()+1, (uint8_t*)gene_id.c_str());
            }
            else
            {
                tmp_c[ret]++;
            }
        }
        if (bc_len > 0)
        {
            memcpy(buf, bam_get_qname(b), bc_len * sizeof(char));
            buf[bc_len] = '\0';
            bam_aux_append(b, c_ptr, 'Z', bc_len+1, (uint8_t*)buf);
        }
        if (UMI_len > 0)
        {
            memcpy(buf, bam_get_qname(b)+bc_len+1, UMI_len * sizeof(char)); // `+1` to add separator
            buf[UMI_len] = '\0';
            bam_aux_append(b, m_ptr, 'Z', UMI_len+1, (uint8_t*)buf);
        }

        bam_aux_append(b, a_ptr, 'i', sizeof(uint32_t), (uint8_t*)&ret);

        int re = sam_write1(of, header, b);
        if (re < 0)
        {
            std::cout << "fail to write the bam file: " << bam_get_qname(b) << std::endl;
            std::cout << "return code: " << re << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    std::cout << "\t" << "unique map to exon:" << tmp_c[0] << std::endl;
    std::cout << "\t" << "ambiguous map to multiple exon:" << tmp_c[1] << std::endl;
    std::cout << "\t" << "map to intron:" << tmp_c[2] << std::endl;
    std::cout << "\t" << "not mapped:" << tmp_c[3] << std::endl;
    std::cout << "\t" << "unaligned:" << unaligned << std::endl;
    sam_close(of);
    bgzf_close(fp);
}

